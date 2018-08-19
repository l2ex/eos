const EosHelper = require('./common/eos-helper')

const eosHelper = new EosHelper()

//eosHelper.getInfo()

if (false) {

    const account = 'account.test'
    const keysPrivate = { owner: '5JKp8AEdkY3RBHTufeRgqjzxuUXzES12fi7QnA9Wm8M1coBKtun', active: '5JX33qxAD5qVq2bhyJy7Bx64g4Atau1BKiAFBpM9JKaxCT87Qse' }
    const keysPublic = { owner: 'EOS6Ae8qZ6NwyED67sYRtYfuU7hBqp6moC3SkW6iRJ54wWUzFR4kQ', active: 'EOS7EZDzfQf9jaW9aJdjUckmqAaBgzHj8WUVqCLBY9Av2UA4ELqkQ' }
    const ramBytes = 1024 * 32
    
    eosHelper.createAccount(account, keysPublic).then(result => {
        console.log()
        console.log(`Created account '${account}':`)
        console.log(result)
    })
    
    eosHelper.buyRAM(account, ramBytes).then(result => {
        console.log()
        console.log(`Bought ${ramBytes} bytes for account '${account}':`)
        console.log(result)
    })
    
    eosHelper.buyBandwidth(account).then(result => {
        console.log()
        console.log(`Bought resources for account '${account}':`)
        console.log(result)
    })

}

if (true) {

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
