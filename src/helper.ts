import fs = require("fs");
import EosEcc = require("eosjs-ecc");
import Eos, { EosInstance } from "eosjs";

export interface AccessProvider {
  publicKey: string;
  privateKey: string;
}

export interface AccountConfig {
  name: string;
  owner: AccessProvider;
  active: AccessProvider;
}

export interface AccountsConfig {
  owner: AccountConfig;
  exchange: AccountConfig;
  token: AccountConfig;
  userA: AccountConfig;
  userB: AccountConfig;
  userC: AccountConfig;
}

export interface HelperConfig {
  eos: {
    config: {
      chainId?: string | null;
      keyProvider: string[];
      httpEndpoint: string;
      expireInSeconds: 60;
      broadcast?: boolean;
      verbose?: boolean;
      sign?: boolean;
    };
  };
  token: {
    symbol: string;
    supply: number;
    decimals: number;
  };
  network: string;
}

// function prepare(raw: AccountsConfig): AccountsConfig {
//   Object.keys(raw).forEach((acc: any) =>
//     ["owner", "active"].forEach(
//       (key: any) =>
//         ((<any>raw)[acc][key].publicKey = EosEcc.PrivateKey(
//           (<any>raw)[acc][key].privateKey
//         )
//           .toPublic()
//           .toString())
//     )
//   );
//   return raw;
// }

export class L2Helper {
  public accounts: AccountsConfig;
  private keys: string[];
  private eos: EosInstance;
  constructor(accounts: AccountsConfig, public config: HelperConfig) {
    this.accounts = accounts;
    this.keys = Object.values(this.accounts)
      .filter(acc => acc.owner && acc.active)
      .map(acc => [acc.owner.privateKey, acc.active.privateKey])
      .reduce((flat, arr) => flat.concat(arr), []);

    // append keys
    this.config.eos.config.keyProvider = this.config.eos.config.keyProvider.concat(
      this.keys
    );

    this.eos = Eos(this.config.eos.config);
  }

  private signAndBroadcast(account: AccountConfig) {
    return {
      authorization: `${account.name}@active`,
      keyProvider: [account.active.privateKey],
      broadcast: true,
      sign: true
    };
  }

  createAccount(
    account: AccountConfig,
    keys: { owner: string; active: string },
    creator: AccountConfig
  ) {
    return (<any>this.eos).transaction((transaction: any) => {
      return transaction.newaccount({
        creator: creator.name,
        name: account.name,
        ...keys
      });
    }, this.signAndBroadcast(creator));
  }

  buyRAM(account: AccountConfig, bytes: number, payer: AccountConfig) {
    return (<any>this.eos).transaction((transaction: any) => {
      return transaction.buyrambytes({
        payer: payer.name,
        receiver: account.name,
        bytes: bytes
      });
    }, this.signAndBroadcast(payer));
  }

  // receiver - account name which will receive bandwidth
  // network - amount of SYS tokens to spend on network usage as number
  // cpu - amount of SYS tokens to spend on CPU usage as number
  buyBandwidth(
    receiver: AccountConfig,
    network: number,
    cpu: number,
    payer: AccountConfig
  ) {
    return (<any>this.eos).transaction((transaction: any) => {
      return transaction.delegatebw({
        from: payer.name,
        receiver: receiver.name,
        stake_net_quantity: `${network}.0000 SYS`,
        stake_cpu_quantity: `${cpu}.0000 SYS`,
        transfer: 0
      });
    }, this.signAndBroadcast(payer));
  }

  setupAccount(
    accKey: keyof AccountsConfig,
    bytes: number,
    net: number,
    cpu: number,
    creatorKey: keyof AccountsConfig
  ) {
    const account = this.accounts[accKey];
    const creator = this.accounts[creatorKey];
    const keys = {
      owner: account.owner.publicKey,
      active: account.active.publicKey
    };

    return this.createAccount(account, keys, creator)
      .then(() => this.buyRAM(account, bytes, creator))
      .then(() => this.buyBandwidth(account, net, cpu, creator));
  }

  deployContract(accKey: keyof AccountsConfig, code: string): any {
    const contract = this.accounts[accKey];
    return this.eos
      .setcode(
        {
          account: contract.name,
          vmtype: 0,
          vmversion: 0,
          code: fs.readFileSync(`./bin/contracts/${code}.wasm`)
        },
        this.signAndBroadcast(contract)
      )
      .then(() =>
        this.eos.setabi(
          {
            account: contract.name,
            abi: JSON.parse(
              fs.readFileSync(`./bin/contracts/${code}.abi`, "utf8")
            )
          },
          this.signAndBroadcast(contract)
        )
      );
  }

  private require_permissions(
    account: string,
    key: string,
    actor: string,
    parent: string
  ) {
    return {
      account: `${account}`,
      permission: "active",
      parent: `${parent}`,
      auth: {
        threshold: 1,
        keys: [
          {
            key: `${key}`,
            weight: 1
          }
        ],
        accounts: [
          {
            permission: {
              actor: `${actor}`,
              permission: "eosio.code"
            },
            weight: 1
          }
        ],
        waits: []
      }
    };
  }

  allowContract(accountKey: keyof AccountsConfig, contractName: string) {
    const account: AccountConfig = this.accounts[accountKey];
    const permission = "active";
    const parent = "owner";
    return this.eos.transaction(
      {
        actions: [
          {
            account: "eosio",
            name: "updateauth",
            authorization: [
              {
                actor: account.name,
                permission: permission
              }
            ],
            data: this.require_permissions(
              account.name,
              account.active.publicKey,
              contractName,
              parent
            )
          }
        ]
      },
      this.signAndBroadcast(account)
    );
  }

  allowContractSignYourself(contractKey: keyof AccountsConfig): any {
    return this.allowContract(contractKey, this.accounts[contractKey].name);
  }

  exchangeInitialize(
    ownerKey: keyof AccountsConfig,
    oracleKey: keyof AccountsConfig
  ) {
    const owner: AccountConfig = this.accounts[ownerKey];
    const oracle: AccountConfig = this.accounts[oracleKey];
    return this.eos.contract(this.accounts.exchange.name).then(contract => {
      return contract.initialize(
        owner.name,
        owner.active.publicKey,
        oracle.name,
        this.signAndBroadcast(owner)
      );
    });
  }

  createToken(): any {
    return this.eos.contract(this.accounts.token.name).then(contract => {
      console.log(
        `${this.config.token.supply}.${Array(this.config.token.decimals)
          .fill(0)
          .join("")} ${this.config.token.symbol}`
      );
      return contract.create(
        {
          issuer: this.accounts.token.name,
          maximum_supply: `${this.config.token.supply}.${Array(
            this.config.token.decimals
          )
            .fill(0)
            .join("")} ${this.config.token.symbol}`
        },
        this.signAndBroadcast(this.accounts.token)
      );
    });
  }

  issueToken(receiverKey: keyof AccountsConfig): any {
    const receiver: AccountConfig = this.accounts[receiverKey];
    return this.eos.contract(this.accounts.token.name).then(contract => {
      return contract.issue(
        {
          to: receiver.name,
          quantity: `${this.config.token.supply}.${Array(
            this.config.token.decimals
          )
            .fill(0)
            .join("")} ${this.config.token.symbol}`,
          memo: "issue"
        },
        this.signAndBroadcast(this.accounts.token)
      );
    });
  }
  getBalance(holderKey: keyof AccountsConfig, symbol: string): any {
    throw new Error("Method not implemented.");
  }
}
