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
        services.linenoise.start(this.handleUserInput)

        services.receive.start()
    }

    async handleUserInput(msg: string):Promise<boolean> {
        let doOneMore = true

        const msgSplit = msg.split(' ')
        switch (msgSplit[0]) {
            case '/exit':
                this.exit()
                doOneMore = false
                break
            case '/name':
                await this.handleSetName(msgSplit[1])
                break
            default:
                // regular text messages
                break
        }

        return doOneMore
    }

    async handleSetName(username: string) {
        const auth = await services.config.get('auth')
        const {auth:authBearer} = await services.api.setOrUpdateName({username, auth})
        if (authBearer) {
            await services.config.set('auth', authBearer)
            await services.config.set('username', username)
        }
    }

    exit() {
        services.linenoise.close()
        services.stdoutee.close()
        services.receive.close()
        services.api.close()
    }
}

export default new ChatnetChatHandler()