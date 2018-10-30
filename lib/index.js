"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const helper_1 = require("./helper");
const config = {
    eos: {
        config: {
            chainId: null,
            keyProvider: (process.env.PRIVATE_KEYS || "").split(","),
            httpEndpoint: process.env.ENDPOINT || "http://localhost:8888"
        }
    },
    token: process.env.TOKEN || "TST",
    network: process.env.NETWORK || "localnet"
};
const accounts = require(`./data/accounts-${config.network}.js`);
async function run() {
    console.log("== Start creating helper");
    const helper = new helper_1.L2Helper(accounts, config);
    console.log("== Start creating accounts");
    await Promise.all(["owner", "exchange", "token", "userA", "userB", "userC"].map(acc => helper
        .setupAccount(acc, 1024 * 1024, 100, 100, "owner")
        .then(() => console.log(`Created ${acc} account`))
        .catch(() => console.log(`Ignore error at ${acc} creation`))));
    console.log("== Deploy token");
    await helper.deployContract("token", "eosio.token");
    console.log("== Deploy exchange");
    await helper.deployContract("exchange", "l2dex");
    console.log("== Setup and initialize l2dex contract");
    await helper.allowContractSignYourself(helper.accounts.exchange);
    await helper.exchangeInitialize(helper.accounts.owner, helper.accounts.userC);
}
run();
