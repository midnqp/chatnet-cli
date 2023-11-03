import services from "@src/services/index.js"

type ChatnetMsg = {
    createdAt?: number | undefined;
    type: "message";
    username: string;
    data: any;
}

class ChatnetReceive {
    constructor() {}

    private isStarted = false

    async start() {
        if (this.isStarted) return

        this.isStarted = true
        services.api.on('broadcast', this.broadcastHandler)
        services.api.on('history', this.historyHandler)
    }

    private broadcastHandler(msg: ChatnetMsg) {
        services.stdoutee.print(msg.data)
    }

    private historyHandler(msgList:Array<ChatnetMsg>) {
        msgList.forEach(msg => services.stdoutee.print(msg.data))
    }

    close() {
        if (!this.isStarted) return

        this.isStarted = false
        services.api.close()
        // todo: mic stop() // but not here, this is for receiving.
        // todo: mic file close()  // but not here, this is for receiving.
        // todo: speaker kill() // yup, you here!
    }
}

export default new ChatnetReceive()