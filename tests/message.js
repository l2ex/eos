const SignatureHelper = require('./common/signature-helper')

const config = require('./common/config')
const accounts = require('./common/accounts')

const keys = accounts.test.active

const channelOwner = accounts.test.name
const symbol = config.token
const change = '7.000000000'
const nonce = 7
const apply = false

const serialized = SignatureHelper.serializeMessage(channelOwner, symbol, change, nonce, apply)
console.log(`Serialized message: ${serialized}`)

const deserialized = SignatureHelper.deserializeMessage(serialized)
//console.log(deserialized)

const messageHash = SignatureHelper.sha256(Buffer.from(serialized))
console.log(`Message hash: ${messageHash.toString('hex')}`)

const signature = SignatureHelper.sign(messageHash, keys.privateKey)
console.log(`Signature: ${signature}`)

const publicKey = SignatureHelper.recover(messageHash, signature)
console.log(`Public key (recovered): ${publicKey}`)
console.log(`Public key (etalon)   : ${keys.publicKey}`)
