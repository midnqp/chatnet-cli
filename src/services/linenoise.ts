import readline from 'node:readline'
import services from '@src/services/index.js'

class ReadInput {
    constructor() { }

    private rl = readline.promises.createInterface(process.stdin, process.stdout)

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
            const doOneMore = await callback(msg)
            if (!doOneMore) break
        }
    }

    close() { return this.rl.close() }
}

export default new ReadInput()