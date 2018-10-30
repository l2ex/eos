import { L2Helper, AccountsConfig, HelperConfig } from "./helper";
const config = {
  eos: {
    config: {
      chainId:
        "038f4b0fc8ff18a4f0842a8f0564611f6e96e8535901dd45e43ac8691a1c4dca",
      keyProvider: (process.env.PRIVATE_KEYS || "").split(","),
      httpEndpoint: process.env.ENDPOINT || "http://localhost:8888"
    }
  },
  token: {
    symbol: process.env.TOKEN_SYMBOL || "DUC",
    decimals: parseInt(process.env.TOKEN_DECIMALS || "4"),
    supply: parseInt(process.env.TOKEN_SUPPLY || "1000000000")
  },
  network: process.env.NETWORK || "localnet"
} as HelperConfig;

const accounts = require(`./data/accounts-${
  config.network
}.js`) as AccountsConfig;

async function run() {
  console.log("== Start creating helper");
  const helper = new L2Helper(accounts, config);

  // console.log("== Start creating accounts");
  // await Promise.all(
  //   ["owner", "exchange", "token", "userA", "userB", "userC"].map(acc =>
  //     helper
  //       .setupAccount(<keyof AccountsConfig>acc, 1024 * 1024, 100, 100, "owner")
  //       .then(() => console.log(`Created ${acc} account`))
  //       .catch(() => console.log(`Ignore error at ${acc} creation`))
  //   )
  // );

  // await helper.buyRAM(
  //   helper.accounts.token,
  //   1024 * 1024,
  //   helper.accounts.token
  // );
  // await helper.buyBandwidth(
  //   helper.accounts.token,
  //   100,
  //   100,
  //   helper.accounts.token
  // );
  console.log("== Deploy token");
  await helper.deployContract("token", "eosio.token");

  // console.log("== Deploy exchange");
  // await helper.deployContract("exchange", "l2dex");

  // console.log("== Setup and initialize l2dex contract");
  // await helper.allowContractSignYourself("exchange");
  // await helper.exchangeInitialize("owner", "userC");

  console.log("== Setup and create token");
  await helper.createToken();
  console.log(`== Token ${config.token.symbol} succesfully created:`);
  await helper.issueToken("owner");
  console.log(`== Token ${config.token.symbol} succesfully issued to owner`);

  // const resultBalanceIssue = await helper.getBalance(
  //   "owner",
  //   helper.config.token.symbol
  // );
  // console.log(`Account owner has ${resultBalanceIssue} tokens`);

  // await eosHelper.transferToken(
  //   symbol,
  //   amountTransfer,
  //   receiverIssue,
  //   receiverTransfer
  // );

  // await eosHelper.transferToken(symbol, "150.000000000", receiverIssue, owner);

  // const resultBalanceTransfer = await eosHelper.getCurrencyBalance(
  //   receiverTransfer.name,
  //   symbol
  // );
  // console.log(`Account ${receiverTransfer.name} has ${resultBalanceTransfer}`);

  // eosHelper.stringToPublicKey(owner.active.publicKey);

  // console.log(`Transfer 30.0 tokens from ${receiverTransfer.name} to l2dex`);
  // await eosHelper.transferToken(
  //   symbol,
  //   "30.000000000",
  //   receiverTransfer,
  //   accounts.l2dex,
  //   eosHelper.stringToPublicKey(receiverTransfer.active.publicKey)
  // );
  // console.log(`Transfer 20.0 tokens from ${receiverTransfer.name} to l2dex`);
  // await eosHelper.transferToken(
  //   symbol,
  //   "20.000000000",
  //   receiverTransfer,
  //   accounts.l2dex,
  //   eosHelper.stringToPublicKey(receiverTransfer.active.publicKey)
  // );

  // console.log(`Transfer 20.0 tokens from ${owner.name} to l2dex`);
  // await eosHelper.transferToken(
  //   symbol,
  //   "20.000000000",
  //   owner,
  //   accounts.l2dex,
  //   eosHelper.stringToPublicKey(owner.active.publicKey)
  // );
  // await eosHelper.transferToken(
  //   symbol,
  //   "50.000000000",
  //   owner,
  //   accounts.l2dex,
  //   eosHelper.stringToPublicKey(owner.active.publicKey)
  // );

  // console.log(
  //   JSON.stringify(
  //     await eosHelper.eos.getTableRows({
  //       code: accounts.l2dex.name,
  //       scope: accounts.l2dex.name,
  //       table: "channels",
  //       json: true
  //     }),
  //     null,
  //     2
  //   )
  // );
  // console.log(
  //   JSON.stringify(
  //     await eosHelper.eos.getTableRows({
  //       code: accounts.l2dex.name,
  //       scope: accounts.l2dex.name,
  //       table: "state",
  //       json: true
  //     }),
  //     null,
  //     2
  //   )
  // );
}

run();
