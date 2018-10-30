const ecc = require("eosjs-ecc");
const EosHelper = require("./common/eos-helper");
const config = require("./common/config");
const accounts = require("./common/accounts-testnet");
const eosHelper = new EosHelper();

async function run() {
  console.log("Deploy exchange");
  await eosHelper.l2dexDeploy();

  console.log("Setup and initialize l2dex contract");
  await eosHelper.allowContract(
    accounts.l2dex.name,
    accounts.l2dex.active.publicKey,
    accounts.l2dex.name
  );

  const owner = accounts.owner;
  const oracle = accounts.userA;
  await eosHelper.l2dexInitialize(owner, oracle);

  console.log(`Transfer 30.0 tokens from ${accounts.userB.name} to l2dex`);
  await eosHelper.transferToken(
    symbol,
    "30.0000",
    accounts.userB,
    accounts.l2dex,
    eosHelper.stringToPublicKey(accounts.userB.active.publicKey)
  );
  console.log(`Transfer 20.0 tokens from ${accounts.userB.name} to l2dex`);
  await eosHelper.transferToken(
    symbol,
    "20.0000",
    accounts.userB,
    accounts.l2dex,
    eosHelper.stringToPublicKey(accounts.userB.active.publicKey)
  );

  console.log(`Transfer 20.0 tokens from ${owner.name} to l2dex`);
  await eosHelper.transferToken(symbol, "20.0000", owner, accounts.l2dex);
  await eosHelper.transferToken(symbol, "50.0000", owner, accounts.l2dex);

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
