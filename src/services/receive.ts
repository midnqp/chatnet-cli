import services from "@src/services/index.js"

type ChatnetMsg = {
    createdAt?: number | undefined;
    type: "message";
    username: string;
    data: any;
}

class ChatnetReceive {
    constructor() {}

    // todo: refactor this code and use the on() method.
    async start() {
        const sio = await services.api.getClient()
        sio.on('broadcast', this.broadcastHandler)
        sio.on('voicemessage', this.voicemessageHandler)
        sio.on('history', this.historyHandler)
    }

    private broadcastHandler(msg: ChatnetMsg) {
        msg.type == 'message' && services.stdoutee.print(msg.data)
    }

    private voicemessageHandler(msg:ChatnetMsg) {}

    private historyHandler(msgList:Array<ChatnetMsg>) {
        msgList.forEach(msg => services.stdoutee.print(msg.data))
    }

    close() {
        //services.api.close()
        // todo: mic stop() // but not here, this is for receiving.
        // todo: mic file close()  // but not here, this is for receiving.
        // todo: speaker kill() // yup, you here!
    }
}

export default new ChatnetReceive()