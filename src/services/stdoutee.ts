import { EventEmitter } from "node:stream"
import services from '@src/services/index.js'

class StdoutEventEmitter {
    constructor() {
        this.eventEmitter.addListener('print', this.printEventListener)
        this.eventEmitter.addListener('clearlineWrite', this.clearlineWriteEventListener)
    }

    private eventEmitter = new EventEmitter({captureRejections:true})

    clearLine() { 
        process.stdout.clearLine(0)
        process.stdout.clearScreenDown()
        process.stdout.cursorTo(0)
    }

    clearlineWrite(msg: string) { return this.eventEmitter.emit('clearlineWrite', msg) }

    print(msg: string) {
        return this.eventEmitter.emit('print', msg)
    }

    close() {
        return this.eventEmitter.removeAllListeners()
    }

    private printEventListener(msg: string) {
        const sentence = services.linenoise.getLine()

        services.stdoutee.clearLine()
        services.linenoise.pause()
        process.stdout.write(msg + '\r\n')

        services.linenoise.writeNewLineWithPrompt(sentence)
    }

    private clearlineWriteEventListener(msg: string) {
        services.stdoutee.clearLine()
        process.stdout.write(msg) // no new line added
    }
}

export default new StdoutEventEmitter()