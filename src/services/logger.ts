import pino from 'pino'
import fs from 'node:fs'
import path from 'node:path'
import os from 'node:os'

class ChatnetLogger {
    constructor() {
        const logFile = path.join(os.homedir(), '.chatnet.log')
        const fileStream = fs.createWriteStream(logFile, 'utf8')
        this.logger = pino({write:fileStream.write})
        this.error = this.logger.error
        this.info = this.logger.info
    }

    private logger

    public error

    public info
}

export default new ChatnetLogger()