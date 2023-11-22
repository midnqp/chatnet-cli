import services from "@src/services/index.js"

let markdownTransform:(s:string)=>string
let markdownToAnsi

/** Await this variable before using ESM packages. */
const importEsmPromise = (async function importEsm() {
    markdownToAnsi  = await import('markdown-to-ansi')
    markdownTransform = markdownToAnsi.default({})
})();

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

        services.logger.info('started listening for messages')
        this.isStarted = true
        services.api.on('broadcast', this.broadcastHandler)
        services.api.on('history', this.historyHandler)
    }

    private async broadcastHandler(msg: ChatnetMsg) {
        await importEsmPromise
        let result = `${msg.username}: ${msg.data}`
        result = markdownTransform(result)
        services.stdoutee.print(result)
    }

    private async historyHandler(msgList:Array<ChatnetMsg>) {
        await importEsmPromise
        msgList.forEach(msg => {
            let result = `${msg.username}: ${msg.data}`
            result = markdownTransform(result)
            services.stdoutee.print(result)
        })
    }

    close() {
        if (!this.isStarted) return
        this.isStarted = false
    }
}

export default new ChatnetReceive()