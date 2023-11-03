const importSocketio = import('socket.io-client')
import services from '@src/services/index.js'

class ChatnetApi {
    constructor() { }

    private serverUrl = 'https://chatnet-server.midnqp.repl.co'

    // do not access, instead use `this.getClient()`
    private _client: any

    private async getClient() {
        const socketio = await importSocketio
        let result: ReturnType<typeof socketio.io>

        if (this._client === undefined) {
            const auth = await services.config.get('auth')
            result = socketio.io(this.serverUrl, { auth: { auth } })
            this._client = result
        }
        else result = this._client

        return result
    }

    public async close() {
        const client = await this.getClient()
        client.close()
    }

    public async setOrUpdateName(opts:{username: string, auth: string}): Promise<{auth:string}> {
        const {username, auth} = opts

        const client = await this.getClient()

        return client.emitWithAck('auth', {
            auth,
            type: 'auth',
            data: username
        })
    }

    public on(eventName:string, callback:(...args: any[]) => void) {
        this.getClient().then(client => client.on(eventName, callback))
    }
}

export default new ChatnetApi()