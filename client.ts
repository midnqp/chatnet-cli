import assert from 'node:assert'
import { inspect, promisify } from 'node:util'
import path from 'node:path'
import socketioClient from 'socket.io-client'
import fs from 'fs-extra'
import lodash from 'lodash'
import recorder from 'node-record-lpcm16'
import { randomUUID } from 'node:crypto'
import { spawn } from 'node:child_process'
import { string } from 'zod'

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
let SPEAKER //= spawn('sox', ['-t', 'wav', '-'])
let io: ReturnType<typeof socketioClient>
const CHATNET_STATE = {
    CONVERSATION_MODE: false,
    // new implementation of mic management!
    MIC: {
        // new files each time, because truly async!!
        FILENAME: '',
        FILE: {},
        REC: {},
        STATUS: 'stopped'
    } as {FILENAME: string, FILE:fs.WriteStream, REC: any, STATUS: 'recording'|'stopped'}
}

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
    process.exit()
})

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
            //await checkConversationMode()
            await emitFromSendQueue()
            await sleep(100)
        }

        io.close()
        if (MICROPHONE !== undefined) MICROPHONE.stop()
        if (MICROPHONEFILE !== undefined) MICROPHONEFILE.close()
        if (SPEAKER?.killed == false) SPEAKER.kill()
        setClientUnavailable()
        logDebug('bye ' + '-'.repeat(30))
    } catch (err) {
        logDebug('fatal crash at main() ' + '-'.repeat(30), err)
        logDebug('bye, force quiting')
        process.exit()
    }
}

// check for nothing, stop everything, just read the audio file and send!
async function stopAndSendRecording({filename,status}) {
    if (status == 'stopped') return
    if (!CHATNET_STATE.CONVERSATION_MODE) CHATNET_STATE.MIC.STATUS = 'stopped' // to prevent getting cleaned twice.

    // just send, that's it
    logDebug('stopAndSendRecording', new Date().toLocaleString())
    const audio = await fs.readFile(filename, {encoding: 'binary'})
    const auth = await configGet('auth');

    io.emitWithAck('voicemessage', {
        auth,
        type: 'voicemessage',
        data: audio
    })
    addToRecvQueue({ data: `🎶 sending voice message`, createdAt: Date.now(), username: 'chatnet', type: 'message' })

    fs.unlink(filename)
}

function startRecordingInLoops() {
    logDebug('startRecordingInLoops', new Date().toLocaleString())

    // stop if conversation mode ended
    const mode = CHATNET_STATE.CONVERSATION_MODE
    if (!mode) return;

    // reset states
    const state = CHATNET_STATE.MIC
    state.FILENAME = '/tmp/chatnet-voicerecord-'+Date.now()
    state.FILE = fs.createWriteStream(state.FILENAME, {encoding: 'binary'})
    state.REC = recorder.record(/*{compress:true}*/)
    state.REC.stream().pipe(state.FILE)
    state.STATUS = 'recording'

    setTimeout(() => {
        CHATNET_STATE.MIC.REC.stop()
        const filename = CHATNET_STATE.MIC.FILENAME
        const status = CHATNET_STATE.MIC.STATUS
        stopAndSendRecording({status, filename}) // deletes old files
        startRecordingInLoops()
    }, 5000)
}

/*
async function checkConversationMode() {
    const mode: boolean = await ipcExec(() => ipcGet("conversationmode"))
    if (mode === CHATNET_STATE.CONVERSATION_MODE) return;
    CHATNET_STATE.CONVERSATION_MODE = mode

    if (mode === true) {
        startRecordingInLoops()
    }
    else {
        // startRecordingInLoops() itself checks 
        // for the mode and quits if false!
    }
}*/

async function playVoiceMessage(msg) {
    const filename = '/tmp/chatnet-audio-received'
    await addToRecvQueue({ data: '🎶 this was a voice message', username: msg.username, type: 'message' })
    await fs.writeFile(filename, msg.data, { encoding: 'binary' })
    spawn('play', [filename])
    // const readStream= fs.createReadStream('/tmp/chatnet-audio-received.wav', {encoding: 'binary'})
    // readStream.pipe(SPEAKER.stdin)
}

/**
 * An idempotent function.
 */
async function turnOffMicrophone(doSend: boolean = false, tooBig = false) {
    if (MICROPHONE && MICROPHONEFILE && MICROPHONEFILENAME) {
        logDebug('turning off microphone', new Date().toLocaleString())
        MICROPHONE.stop()
        if (doSend) {
            const auth = await configGet('auth');
            logDebug("microphone stopped, sending voicemessage")
            const data = await fs.readFile(MICROPHONEFILENAME, { encoding: 'binary' })
            // TODO refactor: actually, add this sendmsgbucket. Single point of truth!
            io.emitWithAck('voicemessage', { // unawaiting, because it takes too much time!
                auth,
                type: 'voicemessage',
                data
            })
            addToRecvQueue({ data: `🎶 sending voice message`, createdAt: Date.now(), username: 'chatnet', type: 'message' })
        } else if (tooBig) {
            addToRecvQueue({ data: `🎶 voice message too big, cancel (try to record for less than 20 sec)`, createdAt: Date.now(), username: 'chatnet', type: 'message' })
        }
        await fs.truncate(MICROPHONEFILENAME, 0)
        MICROPHONE = undefined
        MICROPHONEFILE = undefined
        MICROPHONEFILENAME = undefined
        logDebug("truncated, and all set to undefined")
    }
}

async function checkVoiceMessage() {
    const microphoneState = await ipcExec(()=>ipcGet("voiceMessage"))

    let state:boolean = false
    if (microphoneState == 'on') state=true
    else if (microphoneState == 'done') state=false
    if (state == CHATNET_STATE.CONVERSATION_MODE) return;
    CHATNET_STATE.CONVERSATION_MODE = state

    if (state) {
        startRecordingInLoops()
    }
    else {
        CHATNET_STATE.MIC.REC.stop()
        const filename = CHATNET_STATE.MIC.FILENAME
        const status = CHATNET_STATE.MIC.STATUS
        stopAndSendRecording({filename, status})
        // startRecordingInLoops checks it.
    }
}

/*
async function checkVoiceMessage() {
    const microphoneState = await ipcExec(() => ipcGet('voiceMessage'))
    if (!microphoneState) return

    logDebug("mic requested to be " + microphoneState, new Date().toLocaleString())
    switch (microphoneState) {
        case 'on':
            if (MICROPHONEFILENAME === undefined) MICROPHONEFILENAME = '/tmp/' + randomUUID()// + ".wav"
            if (MICROPHONEFILE === undefined) MICROPHONEFILE = fs.createWriteStream(MICROPHONEFILENAME, { encoding: 'binary' })
            if (MICROPHONE === undefined) {
                MICROPHONE = recorder.record({ compress: true, audioType: 'wav' })
                MICROPHONE.stream().pipe(MICROPHONEFILE)
                logDebug('recording audio in file', MICROPHONEFILENAME)
            }
            setTimeout(() => turnOffMicrophone(false, true), 20000) // auto cancel after 10sec #audio-trim 🧠
            break
        case 'resume':
            MICROPHONE.resume()
            break
        case 'pause':
            MICROPHONE.pause()
            break
        case 'done':
            await turnOffMicrophone(true)
            break
        case 'cancel':
            await turnOffMicrophone(false)
            break
        default:
            addToRecvQueue({ username: 'chatnet', data: 'a value value is one of: **on**, **pause**, **resume**, **done**, **cancel**', type: 'message' })
            break
    }

    await ipcExec(() => ipcPut('voiceMessage', undefined))
}
*/

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
    const str = '[sio-client ' + process.pid + ']  ' + d + '  ' + result + '\n'
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

    if (logDebugIf('sioclient-ping')) logDebug(`toLoop, result is ${result} and lastPingCclient was ${lastpingAgo}ms ago`)
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
type SioMessage = { createdAt?: number, type: 'message'; username: string; data: any }
type SioMessageSend = { type: string, auth: string, data: any }
type SioMeta = { type: 'file'; name: string; data: string }
