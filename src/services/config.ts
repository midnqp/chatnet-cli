import fs from 'node:fs/promises'
import os from 'node:os'
import path from 'node:path'
import services from '@src/services/index.js'

class Conf {
    constructor() {}

    private confPath=path.join(os.homedir(), '.chatnet')

    private defaultConf = {
        auth: '',
        username: ''
    }

    private async ensureConfPath() {
        const found = await services.common.fileFound(this.confPath)
        if (!found) {
            const json = JSON.stringify(this.defaultConf)
            await fs.writeFile(this.confPath, json)
        }
    }

    async set(key: string, value:any) {
        await this.ensureConfPath()
        const json = JSON.parse(await fs.readFile(this.confPath, 'utf8'))
        json[key] = value
        await fs.writeFile(this.confPath, JSON.stringify(json))
    }

    async get(key:string) {
        await this.ensureConfPath()
        const json = JSON.parse(await fs.readFile(this.confPath, 'utf8'))
        return json[key]
    }
}

export default new Conf()