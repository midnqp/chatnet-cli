const importSocketio = import('socket.io-client')
import services from '@src/services/index.js'

/**
 * This class contains API calls to Chatnet server.
 * 
 * Empty constructor. Instantiate using `static async init()`.
 */
class ChatnetApi {
    constructor() {    }

    private serverUrl = 'https://chatnet-server.midnqp.repl.co'

    // do not access, instead use `this.getClient()`
    private _client:any

    async getClient() {
        const socketio = await importSocketio
        let result :ReturnType<typeof socketio.io>

        if (this._client === undefined) {
            const auth = await services.config.get('auth')
            result = socketio.io(this.serverUrl, {auth: {auth}})
            this._client = result
        }
        else result = this._client

        return result
    }

    async close() {
        const client = await this.getClient()
        client.close()
    }

    // todo refactor this, only make the api call and return the ack response from this function.
    async setOrUpdateName(username: string) {
        const client = await this.getClient()
        const auth:string|undefined = await services.config.get('auth')

        const {auth:authBearer} = await client.emitWithAck('auth', { 
            auth, type: 'auth', data: username 
        })
        if (authBearer == '') {
            // username is taken or invalid.
            // server validates and sends msg to user.
        }
        else {
            await services.config.set('auth', authBearer)
            await services.config.set('username', username)
        }
    }

    onBroadcast(cb:Function) {}

    onHistory(cb:Function) {}

    onVoicemessage(cb:Function) {}
}

export default new ChatnetApi()