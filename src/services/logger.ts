import pino from 'pino'
import fs from 'node:fs'
import path from 'node:path'
import os from 'node:os'

class ChatnetLogger {
    private logger = pino(pino.destination(path.join(os.homedir(), '.chatnet.log')))

    private nullLogger = pino({enabled:false})

    public error(...msg:any) {
        if (!process.env.CHATNET_DEBUG) return 
        msg = msg.join(' ')
        this.logger.error(msg)
    }

    public info(...msg:any){
        if (!process.env.CHATNET_DEBUG) return
        msg=msg.join(' ')
        this.logger.info(msg)
    }
}

export default new ChatnetLogger()