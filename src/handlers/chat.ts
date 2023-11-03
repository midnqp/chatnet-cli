import services from '@src/services/index.js'

class ChatnetChatHandler {
    constructor() { }
    
    action() {
        services.linenoise.getPrompt = async () => {
            let result

            const username = await services.config.get('username')
            const name = username || '<username-not-set>'
            result = name + ': '

            return result
        }
        services.linenoise.start(this.handleMsg)

        services.receive.start()
    }

    async handleMsg(msg: string) {
        let doOneMore = true

        const msgSplit = msg.split(' ')
        switch (msgSplit[0]) {
            case '/exit':
                this.exit()
                doOneMore = false
                break
            case '/name':
                await this.handleSetName(msgSplit[1]) // await this one.
                break
            default:
                // regular text messages
                break
        }

        return doOneMore
    }

    async handleSetName(name: string) {
        // todo refactor
        await services.api.setOrUpdateName(name)
    }

    exit() {
        services.linenoise.close()
        services.stdoutee.close()
        services.receive.close()
        services.api.close()
    }
}

export default new ChatnetChatHandler()