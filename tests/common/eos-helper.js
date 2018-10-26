const fs = require('fs')
const Eos = require('eosjs')
const EosEcc = require('eosjs-ecc')

const config = require('./config')
const accounts = require('./accounts')


class EosHelper {

    constructor() {
        config.eos.config.keyProvider.push(accounts.first.owner.privateKey)
        this.eos = Eos(config.eos.config)
        this.contracts = {
            token: {
                abi: fs.readFileSync('./bin/contracts/eosio.token.abi'),
                wasm: fs.readFileSync('./bin/contracts/eosio.token.wasm')
            },
            l2dex: {
                abi: fs.readFileSync('./bin/contracts/l2dex.abi'),
                wasm: fs.readFileSync('./bin/contracts/l2dex.wasm')
            }
        }
        this.options = {
            eos: {
                authorization: 'eosio@active',
                keyProvider: [ '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3' ],
                broadcast: true,
                sign: true
            },
            token: {
                authorization: accounts.token.name,
                keyProvider: [ accounts.token.owner.privateKey ],
                broadcast: true,
                sign: true
            },
            l2dex: {
                authorization: `${accounts.l2dex.name}@active`,
                keyProvider: [ accounts.l2dex.active.privateKey ],
                broadcast: true,
                sign: true
            }
        }
    }

    // Blockchain reading

    getInfo() {
        return this.eos.getInfo({})
    }

    getCurrencyBalance(account, symbol) {
        return this.eos.getCurrencyBalance(accounts.token.name, account, symbol)
    }

    // Accounts management

    createAccount(account, keys) {
        return this.eos.transaction(transaction => {
            return transaction.newaccount({
                creator: 'eosio',
                name: account,
                owner: keys.owner,
                active: keys.active
            })
        }, this.options.eos)
    }

    updatePermissions(account) {
        return this.eos.getAccount(account.name).then(a => {
            const permissions = JSON.parse(JSON.stringify(a.permissions))
            for (const p of permissions) {
                if (p.perm_name == 'active') {
                    // TODO: Check if already added
                    const auth = p.required_auth
                    auth.accounts.push({ permission: { actor: accounts.l2dex.name, permission: 'eosio.code' }, weight: 1 })
                    return this.eos.updateauth({
                        account: account.name,
                        permission: p.perm_name,
                        parent: p.parent,
                        auth: auth
                    }, this.getOptionsOwner(account))
                }
            }
        })
    }

    buyRAM(account, bytes) {
        return this.eos.transaction(transaction => {
            return transaction.buyrambytes({
                payer: 'eosio',
                receiver: account,
                bytes: bytes
            })
        }, this.options.eos)
    }

    // receiver - account name which will receive bandwidth
    // network - amount of SYS tokens to spend on network usage as number
    // cpu - amount of SYS tokens to spend on CPU usage as number
    buyBandwidth(receiver, network, cpu) {
        return this.eos.transaction(transaction => {
            return transaction.delegatebw({
                from: 'eosio',
                receiver: receiver,
                stake_net_quantity: `${network}.0000 SYS`,
                stake_cpu_quantity: `${cpu}.0000 SYS`,
                transfer: 0
            })
        }, this.options.eos)
    }

    // Contract management

    tokenDeploy() {
        return this.eos.setcode(accounts.token.name, 0, 0, this.contracts.token.wasm, this.options.token).then(result => {
            //console.log(result)
            return this.eos.setabi(accounts.token.name, JSON.parse(this.contracts.token.abi), this.options.token).then(result => {
                //console.log(result)
            })
        })
    }

    // symbol - symbol of creating currency as string
    // supply - amount of supplying currency with precision after point (important) as string
    createToken(symbol, supply) {
        return this.eos.contract(accounts.token.name).then(contract => {
            return contract.create(accounts.token.name, `${supply} ${symbol}`, this.options.token)
        })
    }

    // symbol - symbol of creating currency as string
    // amount - amount of issuing currency with precision after point (important) as string
    // receiver - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    issueToken(symbol, amount, receiver) {
        return this.eos.contract(accounts.token.name).then(contract => {
            return contract.issue(receiver.name, `${amount} ${symbol}`, 'issue', this.options.token)
        })
    }

    // symbol - symbol of creating currency as string
    // amount - amount of transferring currency with precision after point (important) as string
    // sender - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // receiver - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    transferToken(symbol, amount, sender, receiver) {
        return this.eos.transfer(sender.name, receiver.name, `${amount} ${symbol}`, 'transfer', this.getOptionsActive(sender))
    }

    // L2dex contract management

    l2dexDeploy() {
        return this.eos.setcode(accounts.l2dex.name, 0, 0, this.contracts.l2dex.wasm, this.options.l2dex).then(result => {
            //console.log(result)
            return this.eos.setabi(accounts.l2dex.name, JSON.parse(this.contracts.l2dex.abi), this.options.l2dex).then(result => {
                //console.log(result)
            })
        })
    }

    // owner - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // oracle - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    l2dexInitialize(owner, oracle) {
        return this.eos.contract(accounts.l2dex.name).then(contract => {
            return contract.initialize(
                owner.name,
                owner.active.publicKey,
                oracle.name,
                this.getOptionsActive(owner)
            )
        })
    }

    // sender - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // receiver - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    l2dexChangeOwner(sender, receiver) {
        return this.eos.contract(accounts.l2dex.name).then(contract => {
            return contract.changeowner(
                receiver.name,
                receiver.active.publicKey,
                this.getOptionsActive(sender)
            )
        })
    }

    // sender - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // symbol - symbol of creating currency as string
    // amount - amount of depositing currency with precision after point (important) as string
    l2dexDeposit(sender, symbol, amount) {
        return this.eos.contract(accounts.l2dex.name).then(contract => {
            return contract.deposit(
                sender.name,
                sender.active.publicKey,
                `${amount} ${symbol}`,
                this.getOptionsActive(sender)
            )
        })
    }

    // receiver - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // symbol - symbol of creating currency as string
    // amount - amount of withdrawing currency with precision after point (important) as string
    l2dexWithdraw(receiver, symbol, amount) {
        return this.eos.contract(accounts.l2dex.name).then(contract => {
            return contract.withdraw(
                receiver.name,
                `${amount} ${symbol}`,
                this.getOptionsActive(receiver)
            )
        })
    }

    // sender - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // ttl - amount of seconds for which channel should be extended from now as number
    l2dexExtendChannel(sender, ttl) {
        return this.eos.contract(accounts.l2dex.name).then(contract => {
            return contract.extend(
                sender.name,
                ttl,
                this.getOptionsActive(sender)
            )
        })
    }

    // sender - object which containt fields: name, owner (contains privateKey, publicKey), active (contains privateKey, publicKey)
    // channelOwner - name of account which owns the channel
    // symbol - symbol of creating currency as string
    // change - amount of currency with precision after point (important) as string
    // nonce - index of off-chain transaction as number
    // apply - boolean flag indicating if transaction should be applied to the channel as boolean
    // signature - signature of the off-chain transaction
    l2dexPushTransaction(sender, channelOwner, symbol, change, nonce, apply, signature) {
        return this.eos.contract(accounts.l2dex.name).then(contract => {
            return contract.update(
                sender.name,
                channelOwner,
                `${change} ${symbol}`,
                nonce,
                apply,
                signature,
                this.getOptionsActive(sender)
            )
        })
    }

    // Elliptic curve cryptography functions (ECC)

    privateKeyToPublicKey(privateKey) {
        return EosEcc.privateToPublic(privateKey)
    }

    sha256(data, encoding) {
        return EosEcc.sha256(data, encoding)
    }

    signData(data, privateKey) {
        return EosEcc.sign(data, privateKey)
    }

    signHash(hash, privateKey) {
        return EosEcc.signHash(hash, privateKey)
    }

    recoverData(data, signature) {
        return EosEcc.recover(signature, data)
    }

    recoverHash(hash, signature) {
        return EosEcc.recoverHash(signature, hash)
    }

    // Helper function

    getOptionsOwner(account) {
        return {
            authorization: `${account.name}@owner`,
            keyProvider: [ account.owner.privateKey ],
            broadcast: true,
            sign: true
        }
    }

    getOptionsActive(account) {
        return {
            authorization: `${account.name}@active`,
            keyProvider: [ account.active.privateKey ],
            broadcast: true,
            sign: true
        }
    }
}

module.exports = EosHelper
