import fs from 'node:fs'
import tmp from 'tmp-promise'

class ChatnetMic {
    constructor() { }

    private _isTurnedOn = false

    get isTurnedOn() { return this._isTurnedOn }

    private set isTurnedOn(value: boolean) { this._isTurnedOn = value }

    private recordingFile?: Omit<tmp.FileResult, 'fd'> & { writeStream: fs.WriteStream }

    async record() {
        if (this.isTurnedOn) throw Error('already recording')

        this.isTurnedOn = true
        const file = await tmp.file()
        const writeStream = fs.createWriteStream(file.path, { encoding: 'binary' })
        this.recordingFile = { path: file.path, cleanup: file.cleanup, writeStream }

    }

    setTimeoutFn(callback: Function, ms: number) {
        setTimeout(callback, ms)
    }

    getAudio(): Buffer {
        return Buffer.alloc(1)
    }

    pause() { }

    stop() { }
}

export default new ChatnetMic()