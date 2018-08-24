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


if (false) {

    const message = 'Hello World!'
    const messageHash = eosHelper.sha256(message)
    console.log(`Message      : ${message}`)
    console.log(`Message hash : 0x${messageHash}`)

    const signatureData = eosHelper.signData(message, '5JKp8AEdkY3RBHTufeRgqjzxuUXzES12fi7QnA9Wm8M1coBKtun')
    const signatureHash = eosHelper.signHash(messageHash, '5JKp8AEdkY3RBHTufeRgqjzxuUXzES12fi7QnA9Wm8M1coBKtun')
    console.log(`Signature calculated from message      : ${signatureData}`)
    console.log(`Signature calculated from message hash : ${signatureHash}`)

    const recoveredPublicKeyData = eosHelper.recoverData(message, signatureData)
    const recoveredPublicKeyHash = eosHelper.recoverHash(messageHash, signatureData)
    console.log(`Public key recovered from signature (data) : ${recoveredPublicKeyData}`)
    console.log(`Public key recovered from signature (hash) : ${recoveredPublicKeyHash}`)
    
}
