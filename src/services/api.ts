const importSocketio = import('socket.io-client')
import services from '@src/services/index.js'

class ChatnetApiService {
    //private client:Promise<ReturnType<Awaited<typeof importSocketio>['io']>>

    constructor() {
        /*  this.client = new Promise(async (resolve, reject) => {
              const socketio = await importSocketio
              const auth = await services.config.get('auth')
  
              resolve(socketio.io(this.serverUrl, {auth:{auth}}))
          })*/
    }


    private serverUrl = 'https://chatnet-server.midnqp.repl.co'

    private client: null | ReturnType<Awaited<typeof importSocketio>['io']> = null

    private notInitializedError: Error = new Error('ChatnetApiService not initialized, or has been already closed. Did you run `init()`?')

    public async init(): Promise<void> {
        const socketio = await importSocketio

        if (this.client) return

        const auth = await services.config.get('auth')
        this.client = socketio.io(this.serverUrl, { auth: { auth } })
    }

    public async close() {
        if (!this.client) return

        const c = this.client
        this.client = null
        await c.close()
    }

    public async makeRequest(data: Object) {
        if (!this.client) throw this.notInitializedError
        // todo: complete this and use this instead.
    }

    // todo: refactor, services should be aloof of any business code
    public async setOrUpdateName(opts: { username: string, auth: string }): Promise<{ auth: string }> {
        const { username, auth } = opts

        if (!this.client) throw this.notInitializedError

        return this.client.emitWithAck('auth', {
            auth,
            type: 'auth',
            data: username
        })
    }

    public on(eventName: string, callback: (...args: any[]) => void) {
        if (!this.client) throw this.notInitializedError
        this.client.on(eventName, callback)
    }
}

export default new ChatnetApiService()