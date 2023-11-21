import pino from 'pino'
import fs from 'node:fs'
import path from 'node:path'
import os from 'node:os'

class ChatnetLogger {
    constructor() {
        const logFile = path.join(os.homedir(), '.chatnet.log')
        this.logger = pino(pino.destination(logFile))
        //this.error = this.logger.error
        //this.info = this.logger.info
    }

    private logger

    public error(...msg:any) {
        msg = msg.join(' ')
        return this.logger.error(msg)
    }

    public info(...msg:any){
        msg=msg.join(' ')
        return this.logger.info(msg)
    }
}

export default new ChatnetLogger()