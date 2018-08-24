const EosHelper = require('./common/eos-helper')
const eosHelper = new EosHelper()

const config = require('./common/config')
const accounts = require('./common/accounts')

const owner = accounts.l2dex
const oracle = accounts.test

const symbol = config.token
const amountDeposit = '10.000000000'


async function go() {

    // Initialize contract
    if (false) {

        const resultInitialize = await eosHelper.l2dexInitialize(owner, oracle)
        console.log('Initializing contract was successful:')
        //console.log(resultInitialize)

    }

    // Change contract owner
    if (false) {

        const resultChangeOwnerA = await eosHelper.l2dexChangeOwner(oracle, oracle)
        console.log('Changing contract owner was successful (forward):')
        //console.log(resultChangeOwnerA)

        const resultChangeOwnerB = await eosHelper.l2dexChangeOwner(oracle, owner)
        console.log('Changing contract owner was successful (backward):')
        //console.log(resultChangeOwnerB)

    }

    // Deposit to the contract
    if (true) {

        const resultDeposit = await eosHelper.l2dexDeposit(accounts.test, symbol, amountDeposit)
        console.log('Deposit to the contract was successful:')
        //console.log(resultDeposit)

    }

}

go()
