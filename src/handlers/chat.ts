import services from '@src/services/index.js'

class ChatnetChatCmd {
    constructor() { }

    exited = false

    readyVoiceCall?: Promise<void>

    async action() {
        services.linenoise.getPrompt = async () => {
            let result

            const username = await services.config.get('username')
            const name = username || '<username-not-set>'
            result = name + ': '

            return result
        }

        await services.api.init()
        services.linenoise.start(this.handleUserInput.bind(this))
        services.receive.start()

        await services.puppeteer.init()

        this.readyVoiceCall = new Promise((async (resolve: Function) => {
            await services.puppeteer.goto('https://chatnet-webrtc-client.midnqp.repl.co')
            const peerjsId = await services.puppeteer.eval(this.puppeteerGetMyPeerjsId)
            await this.handleSetPeerjsId(peerjsId)
            services.logger.info(`okay guys, we're ready for call`)
            resolve()
        }).bind(this))

    }

    puppeteerGetMyPeerjsId() {
        const e = 'chatnet:connected'
        const func = (resl: Function) => window.addEventListener(e, (ev: any) => resl(ev.detail.peerId))
        return new Promise(func)
    }

    async handleSetPeerjsId(id: unknown | string) {
        services.logger.info('my peerjs id is', id)
        services.api.makeRequest('auth-metadata', { data: { peerjsId: id } })
    }


    async handleUserInput(msg: string): Promise<boolean> {
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
            case '/call':
                this.handleCall(msgSplit[1])
                break
            default:
                // regular text messages
                if (!msg) break

                this.handleMessages(msg)
                break
        }

        return doOneMore
    }

    async handleMessages(msg: string) {
        services.api.makeRequest('message', {  type: 'message', data: msg })
    }

    async handleCall(username: string) {
        services.logger.info('making voice call to username:', username)
        await this.readyVoiceCall

        const response = await services.api.makeRequest('get-data', {data: { about: 'user-metadata', key: username } })
        if (!response || !response?.data) {
            services.stdoutee.print(`couldn't call ` + username)
            return
        }        
        services.logger.info('make voice call to peerjs of id:', response.data)
        
        await services.puppeteer.eval([
            `const eventInitDict = {detail:{peerId: '${response.data}'}};`,
            `const event = new window.CustomEvent('chatnet:call:incoming', eventInitDict);`,
            `window.dispatchEvent(event);`
        ].join('\n'))
    }

    async handleSetName(username: string) {
        const { auth: authBearer } = await services.api.makeRequest('auth', { username })
        if (authBearer) {
            await services.config.set('auth', authBearer)
            await services.config.set('username', username)
        }
    }

    async exit() {
        if (this.exited) return

        this.exited = true
        services.linenoise.close()
        services.receive.close()
        services.stdoutee.close()
        
        this.readyVoiceCall?.then(async () => {
            await services.puppeteer.close()
            services.api.close()
        })
    }
}

export default new ChatnetChatCmd()