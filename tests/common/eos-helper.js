const fs = require('fs')
const Eos = require('eosjs')
const EosApi = require('eosjs-api')
const EosEcc = require('eosjs-ecc')

const config = require('./config')
const accounts = require('./accounts')


class EosHelper {

    constructor() {
        config.eos.config.keyProvider.push(accounts.first.owner.wif)
        this.eos = Eos(config.eos.config)
        this.contracts = {

        }
    }

    // Blockchain reading

    getInfo() {
        return this.eos.getInfo({}).then(info => {
            console.log(`Current block: ${info.head_block_num}`)
        })
    }

    // Accounts management

    createAccount(account, keys) {
        const options = {
            authorization: `eosio@active`,
            keyProvider: [ '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3' ],
            broadcast: true,
            sign: true
        }
        return this.eos.transaction(transaction => {
            return transaction.newaccount({
                creator: 'eosio',
                name: account,
                owner: keys.owner,
                active: keys.active
            })
        }, options)
    }

    buyRAM(account, bytes) {
        const options = {
            authorization: `eosio@active`,
            keyProvider: [ '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3' ],
            broadcast: true,
            sign: true
        }
        return this.eos.transaction(transaction => {
            return transaction.buyrambytes({
                payer: 'eosio',
                receiver: account,
                bytes: bytes
            })
        }, options)
    }

    buyBandwidth(account) {
        const options = {
            authorization: `eosio@active`,
            keyProvider: [ '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3' ],
            broadcast: true,
            sign: true
        }
        return this.eos.transaction(transaction => {
            return transaction.delegatebw({
                from: 'eosio',
                receiver: account,
                stake_net_quantity: '100.0000 SYS',
                stake_cpu_quantity: '100.0000 SYS',
                transfer: 0
            })
        }, options)
    }

    // Contract management

    createToken(account, supply, symbol) {
        const permission = 'active'
        const wif = account.active.wif
        const options = {
            authorization: `${account}@${permission}`,
            keyProvider: [ wif ],
            broadcast: true,
            sign: true
        }
        // eos.transfer('alice', 'bob', '1.0000 SYS', '', options)
    }

    // L2dex contract management

    deployL2dex() {

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
}

module.exports = EosHelper
