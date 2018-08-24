const EosHelper = require('./common/eos-helper')
const eosHelper = new EosHelper()

const config = require('./common/config')
const accounts = require('./common/accounts')

const symbol = config.token
const tokenHolderName = accounts.test.name


async function go() {

    await eosHelper.getInfo().then(info => {
        //console.log(info)
        console.log(`Current block: ${info.head_block_num}`)
    })
    
    await eosHelper.getCurrencyBalance(tokenHolderName, symbol).then(balance => {
        console.log(`Account ${tokenHolderName} has ${balance || `no ${symbol}`}`)
    })
}

go()
