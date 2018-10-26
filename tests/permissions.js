const EosHelper = require('./common/eos-helper')
const eosHelper = new EosHelper()

const accounts = require('./common/accounts')


async function go(account) {
    
    const resultUpdatePermissions = await eosHelper.updatePermissions(account)
    console.log(`Extended permissions for '${account.name}' to be used in l2dex contract:`)
    //console.log(resultUpdatePermissions)

}

go(accounts.l2dex)
go(accounts.test)
