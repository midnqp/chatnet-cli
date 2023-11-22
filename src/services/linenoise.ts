import readline from 'node:readline'
import services from '@src/services/index.js'

class ReadInput {
    constructor() { }

    private rl = readline.promises.createInterface(process.stdin, process.stdout, undefined, true)

    public prompt = '> '

    /**
     * Only used in `this.ask()` - not used in `this.refresh()`
     * and `this.writeNewLineWithPrompt()`.
     */
    public getPrompt = async () => this.prompt

    getLine():string {
        return this.rl.line
    }

    getCursor():number {
        return this.rl.cursor
    }

    write(msg:string) { return this.rl.write(msg) }

    pause() { return this.rl.pause() }

    resume() { return this.rl.resume() }

    writeNewLine(sentence:string) {
        // @ts-ignore
        this.rl.line = ''
        // @ts-ignore
        this.rl.cursor = 0
        this.resume()
        this.write(sentence)
    }

    /**
     * Assumes a cleared line and refreshes/rebuilds
     * according to the prompt and line.
     */
    writeNewLineWithPrompt(sentence:string) {
        this.rl.pause()
        process.stdout.write(this.prompt)
        this.writeNewLine(sentence)
    }

    async refresh(savedLine?:string) {
        this.rl.pause()
        const sentence = savedLine !== undefined? savedLine:this.rl.line
        services.stdoutee.clearLine()
        this.writeNewLineWithPrompt(sentence)
    }

    onKeypress(key: string, cb: Function) {
        process.stdin.on('keypress', (char, key) => {
            if (key.name == 'tab') cb()
        })
    }

    /**
     * Ask input for a single prompt.
     */
    async ask() {
        const prompt = await this.getPrompt()
        this.prompt = prompt
        return this.rl.question(prompt)
    }

    /**
     * Start input REPL.
     */
    async start(callback: (msg: string) => boolean | Promise<boolean>) {
        while (true) {
            const msg = await this.ask()
            const promptLen = this.prompt.length
            const len = msg.length + promptLen
            let [x,y] = [0,-1]
            //if (len > process.stdout.columns) {
                const math = len / process.stdout.columns
                const numLines = Math.floor(math)
                const decimalPart = math - numLines
                services.logger.info({decimalPart, numLines, len, promptLen, columns: process.stdout.columns, msg})
                //if (decimalPart === 0) y-- // Last line is filled to brink and the cursor was in new line at x=0 // not needed
                y = y + -numLines
           //}
            process.stdout.moveCursor(x,y)
            for (let i=0; i >= y; i--) process.stdout.clearLine(0)
            
            
            const doOneMore = await callback(msg)
            if (!doOneMore) break
        }
    }

    close() { return this.rl.close() }
}

export default new ReadInput()