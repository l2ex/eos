"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const fs = require("fs");
const eosjs_1 = __importDefault(require("eosjs"));
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
class L2Helper {
    constructor(accounts, config) {
        this.config = config;
        this.accounts = accounts;
        this.keys = Object.values(this.accounts)
            .filter(acc => acc.owner && acc.active)
            .map(acc => [acc.owner.privateKey, acc.active.privateKey])
            .reduce((flat, arr) => flat.concat(arr), []);
        // append keys
        this.config.eos.config.keyProvider = this.config.eos.config.keyProvider.concat(this.keys);
        this.eos = eosjs_1.default(this.config.eos.config);
    }
    signAndBroadcast(account) {
        return {
            authorization: `${account.name}@active`,
            keyProvider: [account.active.privateKey],
            broadcast: true,
            sign: true
        };
    }
    createAccount(account, keys, creator) {
        return this.eos.transaction((transaction) => {
            return transaction.newaccount({
                creator: creator.name,
                name: account.name,
                ...keys
            });
        }, this.signAndBroadcast(creator));
    }
    buyRAM(account, bytes, payer) {
        return this.eos.transaction((transaction) => {
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
    buyBandwidth(receiver, network, cpu, payer) {
        return this.eos.transaction((transaction) => {
            return transaction.delegatebw({
                from: payer.name,
                receiver: receiver.name,
                stake_net_quantity: `${network}.0000 SYS`,
                stake_cpu_quantity: `${cpu}.0000 SYS`,
                transfer: 0
            });
        }, this.signAndBroadcast(payer));
    }
    setupAccount(accKey, bytes, net, cpu, creatorKey) {
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
    deployContract(accKey, code) {
        const contract = this.accounts[accKey];
        return this.eos
            .setcode({
            account: contract.name,
            vmtype: 0,
            vmversion: 0,
            code: fs.readFileSync(`./bin/contracts/${code}.wasm`)
        }, this.signAndBroadcast(contract))
            .then(() => this.eos.setabi({
            account: contract.name,
            abi: JSON.parse(fs.readFileSync(`./bin/contracts/${code}.abi`, "utf8"))
        }, this.signAndBroadcast(contract)));
    }
    require_permissions(account, key, actor, parent) {
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
    allowContract(account, contractName) {
        const permission = "active";
        const parent = "owner";
        return this.eos.transaction({
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
                    data: this.require_permissions(account.name, account.active.publicKey, contractName, parent)
                }
            ]
        }, this.signAndBroadcast(account));
    }
    allowContractSignYourself(exchange) {
        return this.allowContract(exchange, exchange.name);
    }
    exchangeInitialize(owner, oracle) {
        return this.eos.contract(this.accounts.exchange.name).then(contract => {
            return contract.initialize(owner.name, owner.active.publicKey, oracle.name, this.signAndBroadcast(owner));
        });
    }
}
exports.L2Helper = L2Helper;
