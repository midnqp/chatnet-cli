import services from '@src/services/index.js'

let micStatus:'nothing'|'started'|'stopped' = 'nothing'

export default async function () {
    services.linenoise.getPrompt = async () => {
        const username = await services.config.get('username')
        const name = username || '<username-not-set>'
        return name +': '
    }
    services.linenoise.start(handleMsg)
    
    services.receive.start()
}

async function handleMsg (msg:string) {
    let doOneMore = true

    const msgSplit = msg.split(' ')
    switch(msgSplit[0]) {
        case '/exit':
            handleExit()
            doOneMore = false
            break
        case '/name':
            await handleSetName(msgSplit[1]) // await this one.
            break
        case '/mic':
            handleMicToggle() // await if you want to show the 🎙️ icon
        default:
            // regular text messages
            break
    }

    return doOneMore
}

async function handleExit() {
    services.linenoise.close()
    services.stdoutee.close()
    services.receive.close()
}

async function handleSetName(name:string) {
    // todo refactor
    await services.api.setOrUpdateName(name)
}

async function handleMicToggle() {
    if (micStatus == 'nothing') {
        micStatus = 'started'
    }
    // todo
}

function recursiveVoiceRecord() {}