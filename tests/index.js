const ecc = require("eosjs-ecc");
const EosHelper = require("./common/eos-helper");
const config = require("./common/config");
const accounts = require("./common/accounts");
const eosHelper = new EosHelper();

async function run() {
  await Promise.all(
    ["token", "l2dex", "test", "first", "second", "third"].map(acc =>
      eosHelper
        .setupAccount(accounts[acc], 1024 * 1024, 100, 100, accounts.eosio.name)
        .then(_ => console.log(`Created account '${accounts[acc].name}'`))
    )
  );

  console.log("Deploy token");
  await eosHelper.tokenDeploy();
  console.log("Deploy exchange");
  await eosHelper.l2dexDeploy();

  console.log("Setup and initialize l2dex contract");
  await eosHelper.allowContract(
    accounts.l2dex.name,
    accounts.l2dex.active.publicKey,
    accounts.l2dex.name
  );

  const owner = accounts.first;
  const oracle = accounts.test;
  await eosHelper.l2dexInitialize(owner, oracle);

  const symbol = config.token;
  const amountSupply = "1000000.000000000";
  const amountIssue = "1000.000000000";
  const amountTransfer = "450.000000000";
  const receiverIssue = accounts.test;
  const receiverTransfer = accounts.second;

  await eosHelper.createToken(symbol, amountSupply);
  console.log(`Token ${symbol} succesfully created:`);
  await eosHelper.issueToken(symbol, amountIssue, receiverIssue);
  console.log(
    `Token ${symbol} succesfully issued to ${
      receiverIssue.name
    } (${amountIssue}):`
  );

  const resultBalanceIssue = await eosHelper.getCurrencyBalance(
    receiverIssue.name,
    symbol
  );
  console.log(`Account ${receiverIssue.name} has ${resultBalanceIssue}`);

  await eosHelper.transferToken(
    symbol,
    amountTransfer,
    receiverIssue,
    receiverTransfer
  );

  await eosHelper.transferToken(symbol, "150.000000000", receiverIssue, owner);

  const resultBalanceTransfer = await eosHelper.getCurrencyBalance(
    receiverTransfer.name,
    symbol
  );
  console.log(`Account ${receiverTransfer.name} has ${resultBalanceTransfer}`);

  eosHelper.stringToPublicKey(owner.active.publicKey);

  console.log(`Transfer 30.0 tokens from ${receiverTransfer.name} to l2dex`);
  await eosHelper.transferToken(
    symbol,
    "30.000000000",
    receiverTransfer,
    accounts.l2dex,
    eosHelper.stringToPublicKey(receiverTransfer.active.publicKey)
  );
  console.log(`Transfer 20.0 tokens from ${receiverTransfer.name} to l2dex`);
  await eosHelper.transferToken(
    symbol,
    "20.000000000",
    receiverTransfer,
    accounts.l2dex,
    eosHelper.stringToPublicKey(receiverTransfer.active.publicKey)
  );

  console.log(`Transfer 20.0 tokens from ${owner.name} to l2dex`);
  await eosHelper.transferToken(
    symbol,
    "20.000000000",
    owner,
    accounts.l2dex,
    eosHelper.stringToPublicKey(owner.active.publicKey)
  );
  await eosHelper.transferToken(
    symbol,
    "50.000000000",
    owner,
    accounts.l2dex,
    eosHelper.stringToPublicKey(owner.active.publicKey)
  );

  console.log(
    JSON.stringify(
      await eosHelper.eos.getTableRows({
        code: accounts.l2dex.name,
        scope: accounts.l2dex.name,
        table: "channels",
        json: true
      }),
      null,
      2
    )
  );
  console.log(
    JSON.stringify(
      await eosHelper.eos.getTableRows({
        code: accounts.l2dex.name,
        scope: accounts.l2dex.name,
        table: "state",
        json: true
      }),
      null,
      2
    )
  );
}

run();
