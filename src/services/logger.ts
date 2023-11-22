import pino from 'pino'
import pinoPretty from 'pino-pretty'
import fs from 'node:fs'
import path from 'node:path'
import os from 'node:os'
import { inspect } from 'node:util'

class ChatnetLogger {
    //private logger = pino(pino.destination(path.join(os.homedir(), '.chatnet.log')))
    private loggerDestination = pino.destination(path.join(os.homedir(), '.chatnet.log'))
    private logger = pino(pinoPretty({destination: this.loggerDestination}))

    private nullLogger = pino({enabled:false})

    public error(...msg:any) {
        if (!process.env.CHATNET_DEBUG) return 
        msg = msg.join(' ')
        this.logger.error(msg)
    }

    public info(...msg:any){
        if (!process.env.CHATNET_DEBUG) return
        
        let result = ''
        for (let m of msg) {
            result += inspect(m)+' '
        }

        this.logger.info(result )
    }
}

export default new ChatnetLogger()