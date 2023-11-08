import services from '@src/services/index.js'

function puppeteerGetMyPeerjsId() {
    const e = 'chatnet:connected'
    const func = (resl: Function) => window.addEventListener(e, (ev: any) => resl(ev.detail.peerId))
    return new Promise(func)
}

async function  handleSetAuthmetadata(id:unknown|string) {
    console.log('got your peer id bro', id)
    const auth = await services.config.get('auth')
    services.api.makeRequest('auth-metadata', { auth, data: {peerjsId: id} })
}

class ChatnetChatCmd {
    constructor() { }

    readyForCall?:Promise<void>


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

        this.readyForCall= new Promise((async(resolve:Function) => {
            await services.puppeteer.goto('https://chatnet-webrtc-client.midnqp.repl.co')
            const peerjsId = await services.puppeteer.eval(puppeteerGetMyPeerjsId)
            await handleSetAuthmetadata(peerjsId)
            console.log(`okay guys, we're ready for call`)
            resolve()
        }))
    
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
                this.handleMessages(msg)
                break
        }

        return doOneMore
    }

    async handleMessages(msg:string) {
        const auth = await services.config.get('auth')
        services.api.makeRequest('message', {auth, type:'message', data:msg})
    }

    async handleCall(username: string) {

        //const userdata =  await services.api.getUserData(username)
        //if (userdata.peerConnId) {
        console.log('calling username:', username)
        await this.readyForCall
        const auth = await services.config.get('auth')
        const response = await services.api.makeRequest('get-data', {auth, data:{ about:'user-metadata', key:username}})
        if (!response || !response?.data) {
            services.stdoutee.print(`couldn't call `+username)
            return
        }
        console.log('target calling peerid', response.data)
        await services.puppeteer.eval([
            `const eventInitDict = {detail:{peerId: '${response.data}'}};`, 
            `const event = new window.CustomEvent('chatnet:call:incoming', eventInitDict);`,
            `window.dispatchEvent(event);`
        ].join(''))
        //}
    }

    async handleSetName(username: string) {
        const auth = await services.config.get('auth')
        const { auth: authBearer } = await services.api.setOrUpdateName({ username, auth })
        if (authBearer) {
            await services.config.set('auth', authBearer)
            await services.config.set('username', username)
        }
    }

    exit() {
        services.linenoise.close()
        services.receive.close()
        services.stdoutee.close()
        services.api.close()
        services.puppeteer.close()
    }
}

export default new ChatnetChatCmd()