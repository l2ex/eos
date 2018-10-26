const EosHelper = require('./common/eos-helper')
const eosHelper = new EosHelper()


async function token() {

    const resultDeploy = await eosHelper.tokenDeploy()
    console.log('Token contract successfully deployed:')
    //console.log(resultDeploy)

}

async function l2dex() {

    const resultDeploy = await eosHelper.l2dexDeploy()
    console.log('L2dex contract successfully deployed:')
    //console.log(resultDeploy)

}

async function go() {

    await token()
    await l2dex()

}

go()
