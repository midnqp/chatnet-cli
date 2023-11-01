class ChatnetMic {
    constructor() {}

    isTurnedOn=false

    micFilename = ''

    start() {}

    setTimeoutFn(callback:Function, ms:number) {
        setTimeout(callback, ms)
    }

    getAudio():Buffer {
        return Buffer.alloc(1)
    }

    stop() {}

    close() {}
}

export default new ChatnetMic()