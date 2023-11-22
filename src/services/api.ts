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

    private notInitErrormsg: string = 'ChatnetApiService not initialized, or has been already closed. Did you run `init()`?'

    public async init(): Promise<void> {
        const socketio = await importSocketio

        if (this.client) return

        const auth = await services.config.get('auth')
        this.client = socketio.io(this.serverUrl, { auth: { auth } })
    }

    public close() {
        if (!this.client) return

        const c = this.client
        this.client = null
        c.close()
    }

    public async makeRequest(channelName:string, data: Record<string, any>) {
        if (!this.client) throw Error(this.notInitErrormsg)

        data['auth'] = await services.config.get('auth')
        const result = this.client.emitWithAck(channelName, data)
        return result
    }

    public on(eventName: string, callback: (...args: any[]) => void) {
        if (!this.client) throw Error(this.notInitErrormsg)
        this.client.on(eventName, callback)
    }
}

export default new ChatnetApiService()