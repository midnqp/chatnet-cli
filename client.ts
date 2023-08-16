import assert from 'node:assert'
import { inspect, promisify } from 'node:util'
import path from 'node:path'
import socketioClient from 'socket.io-client'
import fs from 'fs-extra'
import lodash from 'lodash'
import recorder from 'node-record-lpcm16'
import { randomUUID } from 'node:crypto'
import { spawn } from 'node:child_process'

const IPCTIMEOUT = 15000 as const // 15 sec
const IPCDIR = getIpcDir()
const IPCPATH = path.join(IPCDIR, 'ipc.json')
const CONFIGPATH = path.join(IPCDIR, '..', '.chatnet.json')
const IPCLOCKF = path.join(IPCDIR, 'LOCK')
const IPCUNLOCKF = path.join(IPCDIR, 'UNLOCK')
const IPCLOGF = path.join(IPCDIR, 'log-latest.txt')
const CLIENTUPF = path.join(IPCDIR, 'CLIENTUP')
const SERVERURL = 'https://chatnet-server.midnqp.repl.co'
let LAST_DEAD_PROBE = 0
let MICROPHONEFILENAME
let MICROPHONEFILE
let MICROPHONE
const SPEAKER = spawn('sox', ['-t', 'wav', '-'])
let io: ReturnType<typeof socketioClient>
main().catch((reason) => {
    logDebug('main().catch', reason)
    logDebug('bye, force quiting')
    process.exit()
})

process.on('uncaughtException', (err, origin) => {
    logDebug("uncaught error", err, origin)
    logDebug('bye, force quiting')
    process.exit()
})

process.on('unhandledRejection', (reason, promise) => {
    logDebug('uncaught promise rejection', reason, promise)
    logDebug('bye, force quiting')
    process.exit()
})

process.on('uncaughtExceptionMonitor', (err, origin) => {
    logDebug('uncaught exception monitor', err, origin)
    logDebug('bye, force quiting')
    process.exit()})

async function main() {
    try {
        assert(IPCDIR != '', 'database not found')
        logDebug('hi')
        setClientAvailable()

        const auth = await configGet('auth')
        io = socketioClient(SERVERURL, { auth: { auth } })
        io.on('history', async (msgList: Array<SioMessage>) => {
            if (msgList.length == 0) return
            logDebug(`received ${msgList} messages as history`)
            //msgList.forEach(addToRecvQueue)
            // maybe i need to add those all at once like this!
            let bucket = await ipcExec(() => ipcGet("recvmsgbucket"))
            bucket = bucket.concat(msgList)
            await ipcExec(() => ipcPut("recvmsgbucket", bucket))
        })
        io.on('broadcast', addToRecvQueue)
        io.on('voicemessage', playVoiceMessage)

        while (await toLoop()) {
            await checkIfAuthChanged()
            await checkVoiceMessage()
            await emitFromSendQueue()
            await sleep(100)
        }

        io.close()
        if (MICROPHONE !== undefined) MICROPHONE.stop()
        if (MICROPHONEFILE !== undefined) MICROPHONEFILE.close()
        if (!SPEAKER.killed) SPEAKER.kill()
        setClientUnavailable()
        logDebug('bye ' + '-'.repeat(30))
    } catch (err) {
        logDebug('fatal crash at main() ' + '-'.repeat(30), err)
        logDebug('bye, force quiting')
        process.exit()
    }
}

async function playVoiceMessage(msg) {
    const filename = '/tmp/chatnet-audio-received.wav'
    await addToRecvQueue({ data: '🎶 this was a voice message', username: msg.username, type: 'message' })
    await fs.writeFile(filename, msg.data, { encoding: 'binary' })
    spawn('play', [filename])
    // const readStream= fs.createReadStream('/tmp/chatnet-audio-received.wav', {encoding: 'binary'})
    // readStream.pipe(SPEAKER.stdin)
}

async function checkVoiceMessage() {
    const microphoneState = await ipcExec(() => ipcGet('voiceMessage'))
    if (!microphoneState) return

    if (MICROPHONEFILENAME === undefined) MICROPHONEFILENAME = '/tmp/' + randomUUID() + ".wav"
    if (MICROPHONEFILE === undefined) MICROPHONEFILE = fs.createWriteStream(MICROPHONEFILENAME, { encoding: 'binary' })

    logDebug("mic requested to be "+microphoneState)
    switch (microphoneState) {
        case 'on':
            if (MICROPHONE === undefined) {
                MICROPHONE = recorder.record()
                MICROPHONE.stream().pipe(MICROPHONEFILE)
            }
            break
        case 'resume':
            MICROPHONE.resume()
            break
        case 'pause':
            MICROPHONE.pause()
            break
        case 'done':
            MICROPHONE.stop()
            //const fileClose = promisify(MICROPHONEFILE.close)
            //await fileClose()
            const auth = await configGet('auth');
            logDebug("file closed, microphone stopped, sending voicemessage")
            io.emitWithAck('voicemessage', { // unawaiting, because it takes too much time!
                auth,
                type: 'voicemessage',
                data: await fs.readFile(MICROPHONEFILENAME, { encoding: 'binary' })
            })
            await fs.truncate(MICROPHONEFILENAME, 0)
            MICROPHONE = undefined
            MICROPHONEFILE = undefined
            MICROPHONEFILENAME = undefined
            logDebug("delivered voicemessage, truncated, and all set to undefined")
            break
        case 'cancel':
            MICROPHONE.stop()
            await fs.truncate(MICROPHONEFILENAME, 0)
            MICROPHONE = undefined
            MICROPHONEFILE = undefined
            MICROPHONEFILENAME = undefined
            break
        default:
            addToRecvQueue({ username: 'chatnet', data: 'a value value is one of: **on**, **pause**, **resume**, **done**, **cancel**', type: 'message' })
            break
    }

    await ipcExec(() => ipcPut('voiceMessage', undefined))
}

async function checkIfAuthChanged() {
    const username = await ipcExec(() => ipcGet('username'))
    if (username !== undefined) {

        const auth = await configGet('auth');
        const { auth: authBearer } = await io.emitWithAck('auth', { auth, type: 'auth', data: username })
        if (authBearer == '') {
            // sad :(
            // "he was last seen 24 days ago"
            await ipcExec(() => ipcPut('username', undefined))
        }
        else {
            await configPut('auth', authBearer)
            await configPut('username', username)
            await ipcExec(() => ipcPut('username', undefined))
        }
    }
}

async function addToRecvQueue(msg: SioMessage) {
    const bucket = await ipcExec(() => ipcGet('recvmsgbucket'))
    const arr = bucket
    //const arr: IpcMsgBucket = JSON.parse(bucket)
    if (!Array.isArray(arr)) return

    arr.push(msg)
    await ipcExec(() => ipcPut('recvmsgbucket', arr))
}

async function emitFromSendQueue() {
    const bucket = await ipcExec(() => ipcGet('sendmsgbucket'))
    //let arr: IpcMsgBucket = JSON.parse(bucket)
    const arr = bucket

    const authBearer = await configGet('auth')

    if (!arr.length) return

    const arrNew = new Array(...arr)
    for (let item of arrNew) {
        if (item.type == 'message') {
            if (authBearer == '') {
                await addToRecvQueue({ data: 'please set username to send messages using: /name <your-name>', type: 'message', username: 'chatnet' })
                lodash.remove(arrNew, item)
                continue
            }

            delete item.username
            item.auth = authBearer
            await io.emitWithAck('message', item)
            lodash.remove(arrNew, item)
        }
    }
    await ipcExec(() => ipcPut('sendmsgbucket', arrNew))
}

// code below are mostly utils

async function configGet(key: string) {
    const json = await fs.readJson(CONFIGPATH)
    return json[key]
}

async function configPut(key: string, val: any) {
    const json = await fs.readJson(CONFIGPATH)
    json[key] = val
    await fs.writeJson(CONFIGPATH, json)
}

// do not use except as such as: await ipcExec(() => ipcGet(key, val))
async function ipcGet(key: string) {
    const tryFn = async () => {
        const str = await fs.readFile(IPCPATH, 'utf8')
        const json: Record<string, any> = JSON.parse(str)
        return json[key]
    }
    const catchFn = e => logDebugIf('sioclient-ipc') && logDebug('ipcGet: something failed: retry', e)

    return retryableRun(tryFn, catchFn)
}

// do not use except as such as: await ipcExec(() => ipcPut(key, val))
async function ipcPut(
    key: string,
    val: Record<string, any> | Array<any> | string | number | undefined
) {
    const tryFn = async () => {
        const str = await fs.readFile(IPCPATH, 'utf8')
        const json: Record<string, any> = JSON.parse(str)
        json[key] = val
        await fs.writeJson(IPCPATH, json)
    }
    const catchFn = e => logDebugIf('sioclient-ipc') && logDebug('ipcPut: something failed: retry', e)

    return retryableRun(tryFn, catchFn)
}

function logDebugIf(scope: string): boolean {
    const scopes = process.env.CHATNET_DEBUG || ''
    const scopeList = scopes.split(',')
    if (scopeList.includes(scope)) return true
    return false
}

/** runtime debug logs */
function logDebug(...any): void {
    if (!process.env.CHATNET_DEBUG) return

    let result = ''
    for (let each of any) result += inspect(each) + ' '
    if (any.length) result.slice(0, result.length - 1)

    const d = new Date().toISOString()
    const str = '[sio-client '+process.pid+']  ' + d + '  ' + result + '\n'
    fs.appendFileSync(IPCLOGF, str)
}

/** checks if event loop should continue running */
async function toLoop() {
    let result = true
    //const userstate = await ipcExec(() => ipcGet('userstate'))
    //if (userstate === false) result = false

    const now = Date.now()
    // if ((now - LAST_DEAD_PROBE) > 0) {
    await ipcExec(() => ipcPut('lastping-sioclient', String(Date.now())))
    const lastPingCclient: string | undefined = await ipcExec(() => ipcGet('lastping-cclient'))

    let lastpingAgo = -1
    if (lastPingCclient?.length) {
        const lastping = parseInt(lastPingCclient)
        lastpingAgo = (now - lastping)
        if (lastpingAgo > 3000) result = false // cclient is probably dead 💀
        LAST_DEAD_PROBE = now
    }
    // }

    logDebug(`toLoop, result is ${result} and lastPingCclient was ${lastpingAgo}ms ago`)
    return result
}

/** promise-based sleep */
function sleep(ms: number) {
    return new Promise(r => setTimeout(r, ms))
}

/** returns folder for IPC based on platform */
function getIpcDir() {
    let p = ''
    if (process.platform == 'linux') {
        const HOME = process.env.HOME
        assert(HOME !== undefined)
        p = path.join(HOME, '.config', '.chatnet-client')
    }
    return p
}

async function setIpcLock() {
    return retryableRun(() => fs.rename(IPCUNLOCKF, IPCLOCKF))
}

async function unsetIpcLock() {
    return retryableRun(() => fs.rename(IPCLOCKF, IPCUNLOCKF))
}

function setClientAvailable() {
    fs.ensureFileSync(CLIENTUPF)
}

function setClientUnavailable() {
    fs.existsSync(CLIENTUPF) && fs.unlinkSync(CLIENTUPF)
}

// Essential wrapper over ipcGet() and ipcPut()
async function ipcExec(fn: () => Promise<any>) {
    const tryFn = async () => {
        const existsLock = await fs.exists(IPCLOCKF)
        const existsUnlock = await fs.exists(IPCUNLOCKF)
        if (!existsLock && existsUnlock) {
            await setIpcLock()
            const result = await fn()
            await unsetIpcLock()
            return result
        } else throw Error('database not reachable')
    }
    const catchFn = () => logDebugIf('sioclient-ipc') && logDebug(`database is locked, retrying`)

    return retryableRun(tryFn, catchFn)
}

/**
 * runs a function, keeps retyring
 * every 50ms on failure, throws
 * error in case of timeout
 */
async function retryableRun(tryFn, catchFn: Function = () => { }) {
    let n = 0
    const maxN = IPCTIMEOUT
    while (true) {
        try {
            return await tryFn()
        } catch (err: any) {
            await catchFn(err)
            if (n > maxN) throw Error(err.message)
            n += 50
            await sleep(50)
        }
    }
}

type IpcMsgBucket = Array<SioMessage>
type SioMessage = { type: 'message'; username: string; data: any }
type SioMessageSend = { type: string, auth: string, data: any }
type SioMeta = { type: 'file'; name: string; data: string }
