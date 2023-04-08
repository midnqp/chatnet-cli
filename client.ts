import assert from 'node:assert'
import { inspect } from 'node:util'
import path from 'node:path'
import socketioclient from 'socket.io-client'
import fs from 'fs-extra'
const serverurl = 'https://chatnet-server.midnqp.repl.co'
const io = socketioclient(serverurl)

const IPCTIMEOUT = 15000 as const
const IPCDIR = getIpcDir()
const IPCPATH = path.join(IPCDIR, 'ipc.json')
const IPCLOCKF = path.join(IPCDIR, 'LOCK')
const IPCUNLOCKF = path.join(IPCDIR, 'UNLOCK')
const IPCLOGF = path.join(IPCDIR, 'log-latest.txt')
const CLIENTUPF = path.join(IPCDIR, 'CLIENTUP')

main()

async function main() {
    if (!IPCDIR) return console.error('database not found')

    logDebug('hi')
    setClientUp()

    try {
        logDebug("listening for 'broadcast'")
        io.on('broadcast', (msg: SioMessage) => {
            const handleNewMsg = async (bucket: string) => {
                const arr: IpcMsgBucket = JSON.parse(bucket)
                if (!Array.isArray(arr)) return
                arr.push(msg)
                const arrstr = JSON.stringify(arr)
                await ipcExec(() => ipcPut('recvmsgbucket', arrstr))
            }
            ipcExec(() => ipcGet('recvmsgbucket')).then(handleNewMsg)
        })

        logDebug('send-msg-loop starting')
        while (await toLoop()) {
            const bucket = await ipcExec(() => ipcGet('sendmsgbucket'))
            let arr: IpcMsgBucket = JSON.parse(bucket)

            if (arr.length) {
                await ipcExec(() => ipcPut('sendmsgbucket', '[]'))
                logDebug('found sendmsgbucket', arr)
                for (let item of arr) {
                    if (item.type == 'message') {
                        logDebug('sending message', item)
                        io.emit('message', item)
                    }
                }
            }

            await sleep(1000)
        }
        logDebug('loop ended, closing socket.io')
        io.close()
    } catch (err) {
        logDebug('something went very wrong', err)
    }
    setClientDown()
    logDebug('bye')
} // ends main()

async function ipcGet(key: string) {
    while (true) {
        try {
            const str = await fs.readFile(IPCPATH, 'utf8')
            logDebug(`ipcGet: ipc.json: `, str)
            const json = JSON.parse(str)
            let value = json[key]
            if (value === undefined) value = null
            return value
        } catch (e) {
            logDebug('ipcGet: something failed: retry', e)
            await sleep(50)
        }
    }
}

async function ipcPut(key: string, val: string) {
    while (true) {
        try {
            const str = await fs.readFile(IPCPATH, 'utf8')
            logDebug(`ipcPut: ipc.json: `, str)
            const json = JSON.parse(str)
            json[key] = val
            await fs.writeJson(IPCPATH, json)
            return
        } catch (e) {
            logDebug('ipcPut: something failed: retry',e)
            await sleep(50)
        }
    }
}

/** runtime debug logs */
function logDebug(...any) {
    if (process.env.CHATNET_DEBUG) {
        let result = ''
        for (let each of any) result += inspect(each) + ' '
        if (any.length) result.slice(0, result.length - 1)

        const d = new Date().toISOString()
        const str = '[sio-client]  ' + d + '  ' + result + '\n'
        fs.appendFileSync(IPCLOGF, str)
    }
}

/** checks if event loop should continue running */
async function toLoop() {
    let result = true
    try {
        const userstate = await ipcExec(() => ipcGet('userstate'))
        if (userstate == 'false') result = false
        //else result = false
    } catch (err) {
        //result = false
        result = true
    }
    return result
}

function sleep(ms: number) {
    return new Promise(r => setTimeout(r, ms))
}

function getIpcDir() {
    let p = ''
    if (process.platform == 'linux') {
        const HOME = process.env.HOME
        assert(HOME !== undefined)
        p = path.join(HOME, '.config', 'chatnet-client')
    }
    return p
}

async function setIpcLock() {
    while (true) {
        try {
            await fs.rename(IPCUNLOCKF, IPCLOCKF)
            break
        } catch (err) {
            await sleep(50)
        }
    }
}

async function unsetIpcLock() {
    while (true) {
        try {
            await fs.rename(IPCLOCKF, IPCUNLOCKF)
            break
        } catch (err) {
            await sleep(50)
        }
    }
}

function setClientUp() {
    fs.ensureFileSync(CLIENTUPF)
}

function setClientDown() {
    fs.unlinkSync(CLIENTUPF)
}

async function ipcExec(fn: () => Promise<any>) {
    let ms = 0
    let result: any
    let fnExecuted = false

    while (ms < IPCTIMEOUT) {
        const existsLock = await fs.exists(IPCLOCKF)
        const existsUnlock = await fs.exists(IPCUNLOCKF)
        if (!existsLock && existsUnlock) {
            await setIpcLock()
            result = await fn()
            fnExecuted = true
            await unsetIpcLock()
            break
        }

        logDebug(`database locked, retrying ... (${ms}/${IPCTIMEOUT}) ms`)
        const waitMs = 100
        ms += waitMs
        await sleep(waitMs)
    }
    if (fnExecuted == false) {
        logDebug('database not reachable')
        setClientDown()
        process.exit()
    }
    return result
}

type IpcMsgBucket = Array<SioMessage>
type SioMessage = { type: 'message'; username: string; data: string }
type SioMeta = { type: 'file'; name: string; data: string }
