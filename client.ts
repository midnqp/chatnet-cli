import assert from 'node:assert'
import { inspect } from 'node:util'
import path from 'node:path'
import socketioclient from 'socket.io-client'
import fs from 'fs-extra'
import childprocess from 'node:child_process'
import recorder from 'node-record-lpcm16'

const IPCTIMEOUT = 15000 as const
const IPCDIR = getIpcDir()
const IPCPATH = path.join(IPCDIR, 'ipc.json')
const IPCLOCKF = path.join(IPCDIR, 'LOCK')
const IPCUNLOCKF = path.join(IPCDIR, 'UNLOCK')
const IPCLOGF = path.join(IPCDIR, 'log-latest.txt')
const CLIENTUPF = path.join(IPCDIR, 'CLIENTUP')
const SERVERURL = 'https://chatnet-server.midnqp.repl.co'

const io = socketioclient(SERVERURL)
const speaker = childprocess.spawn('sox', ['-t', 'wav', '-', '-d'])
const microphone = recorder.record({ sampleRate: 8000, compress: true })

microphone.pause()
io.connect()
main()

async function main() {
    assert(!IPCDIR, 'database not found')
    logDebug('hi')
    setClientAvailable()

    try {
        logDebug("listening for 'broadcast'")
        io.on('broadcast', (msg: SioMessage) => {
            const promise = ipcExec(() => ipcGet('recvmsgbucket'))
            promise.then(bucket => addToRecvQueue(bucket, msg))
        })

        logDebug('send-msg-loop starting')
        while (await toLoop()) {
            const bucket = await ipcExec(() => ipcGet('sendmsgbucket'))
            await emitFromSendQueue(bucket)

            await emitFromMicrophone()

            await sleep(1000)
        }
    } catch (err) {
        logDebug('something went very wrong', err)
    }

    logDebug('closing socket.io')
    io.close()
    setClientUnavailable()
    logDebug('bye')
} // ends main()

async function emitFromMicrophone() {
    const micstate = await ipcExec(() => ipcGet('micstate'))
    if (micstate == 'on') {
        microphone.stream().on('data', buf => io.emit('voice:in', buf))
        io.on('voice:out', buf => speaker.stdin.write(buf))
    }
    if (micstate == 'mute') microphone.pause()
    if (micstate == 'off') {
        microphone.stop()
        io.off('voice:out')
    }
}

async function addToRecvQueue(bucket: string, msg: SioMessage) {
    const arr: IpcMsgBucket = JSON.parse(bucket)
    if (!Array.isArray(arr)) return

    arr.push(msg)
    const arrstr = JSON.stringify(arr)
    await ipcExec(() => ipcPut('recvmsgbucket', arrstr))
}

async function emitFromSendQueue(bucket: string) {
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
}

/************** code below are mostly utils **************/

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
            logDebug('ipcPut: something failed: retry', e)
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

function setClientAvailable() {
    fs.ensureFileSync(CLIENTUPF)
}

function setClientUnavailable() {
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
        setClientUnavailable()
        process.exit()
    }
    return result
}

type IpcMsgBucket = Array<SioMessage>
type SioMessage = { type: 'message'; username: string; data: string }
type SioMeta = { type: 'file'; name: string; data: string }
