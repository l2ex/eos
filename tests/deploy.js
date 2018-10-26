const EosHelper = require('./common/eos-helper')
const eosHelper = new EosHelper()


async function go() {

    const resultDeploy = await eosHelper.l2dexDeploy()
    console.log('L2dex contract successfully deployed:')
    console.log(resultDeploy)

}

go()
