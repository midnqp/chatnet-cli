import services from '@src/services/index.js'

let micStatus = false

export default async function () {
    services.linenoise.getPrompt = async () => {
        let result = services.linenoise.prompt

        const username = await services.config.get('username')
        const name = username || '<username-not-set>'
        result = name + ': '

        return result
    }
    services.linenoise.start(handleMsg)

    services.receive.start()
}

async function handleMsg(msg: string) {
    let doOneMore = true

    const msgSplit = msg.split(' ')
    switch (msgSplit[0]) {
        case '/exit':
            handleExit()
            doOneMore = false
            break
        case '/name':
            await handleSetName(msgSplit[1]) // await this one.
            break
        case '/mic':
            await handleMicToggle() // await if you want to show the 🎙️ icon
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

async function handleSetName(name: string) {
    // todo refactor
    await services.api.setOrUpdateName(name)
}

async function handleMicToggle() {
    if (micStatus == false) {
        await services.mic.start()

        services.mic.setTimeoutFn(function () {
            services.mic.stop()
            const audio = services.mic.getAudio()
            services.api.sendVoicemessage(audio)
        }, 5000)
    }
    else {
        services.mic.stop()
        const audio = services.mic.getAudio()
        services.api.sendVoicemessage(audio)
        services.mic.close()
    }

    services.mic.isTurnedOn = !services.mic.isTurnedOn
}

function recursiveVoiceRecord() { }