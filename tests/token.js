const EosHelper = require('./common/eos-helper')
const eosHelper = new EosHelper()

const config = require('./common/config')
const accounts = require('./common/accounts')

const symbol = config.token
const amountSupply = '1000000.000000000'
const amountIssue = '1000.000000000'
const amountTransfer = '450.000000000'
const receiverIssue = accounts.test
const receiverTransfer = accounts.first


async function go() {
    
    const resultCreateToken = await eosHelper.createToken(symbol, amountSupply)
    console.log(`Token ${symbol} succesfully created:`)
    //console.log(resultCreateToken)
    
    const resultIssueToken = await eosHelper.issueToken(symbol, amountIssue, receiverIssue)
    console.log(`Token ${symbol} succesfully issued to ${receiverIssue.name} (${amountIssue}):`)
    //console.log(resultIssueToken)
    
    const resultBalanceIssue = await eosHelper.getCurrencyBalance(receiverIssue.name, symbol)
    console.log(`Account ${receiverIssue.name} has ${resultBalanceIssue}`)

    const resultTransfer = await eosHelper.transferToken(symbol, amountTransfer, receiverIssue, receiverTransfer)
    //console.log(resultTransfer)
    
    const resultBalanceTransfer = await eosHelper.getCurrencyBalance(receiverTransfer.name, symbol)
    console.log(`Account ${receiverTransfer.name} has ${resultBalanceTransfer}`)
}

go()
