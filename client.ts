import assert from 'node:assert'
import { inspect } from 'node:util'
import path from 'node:path'
import socketioClient from 'socket.io-client'
import fs from 'fs-extra'
import lodash from 'lodash'

const IPCTIMEOUT = 15000 as const // 15 sec
const IPCDIR = getIpcDir()
const IPCPATH = path.join(IPCDIR, 'ipc.json')
const CONFIGPATH = path.join(IPCDIR, '..', '.chatnet.json')
const IPCLOCKF = path.join(IPCDIR, 'LOCK')
const IPCUNLOCKF = path.join(IPCDIR, 'UNLOCK')
const IPCLOGF = path.join(IPCDIR, 'log-latest.txt')
const CLIENTUPF = path.join(IPCDIR, 'CLIENTUP')
const SERVERURL = 'https://chatnet-server.midnqp.repl.co'
const io = socketioClient(SERVERURL)
main()

async function main() {
    assert(IPCDIR != '', 'database not found')
    logDebug('hi')
    setClientAvailable()

    io.on('broadcast', addToRecvQueue)

    while (await toLoop()) {
        await checkIfAuthChanged()
        await emitFromSendQueue()
        await sleep(500)
    }

    io.close()
    setClientUnavailable()
    logDebug('bye')
}

async function checkIfAuthChanged() {
    const username = await ipcExec(()=>ipcGet('username'))
    if (username) {
        const auth = await configGet('auth');
        const {auth:authBearer} = await io.emitWithAck('auth', {auth, type:'auth', data: username})
        if (authBearer == '') {
            // sad :(
            // "he was last seen 24 days ago"
            await addToRecvQueue({data: 'sorry, someone with this username is already signed up :(', type: 'message', username: 'chatnet'})
            await ipcPut('username', undefined)
        }
        else {
            await configPut('auth', authBearer)
            await configPut('username', username)
            await ipcPut('username', undefined)
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
                await addToRecvQueue({data: 'please set username to send messages using: /name <your-name>', type: 'message', username: 'chatnet'})
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

async function configGet(key:string) {
    const json = await fs.readJson(CONFIGPATH)
    return json[key]
}

async function configPut(key:string, val:any) {
    const json = await fs.readJson(CONFIGPATH)
    json[key] = val
    await fs.writeJson(CONFIGPATH, json)
}

// do not use except as such as: await ipcExec(() => ipcGet(key, val))
async function ipcGet(key: string) {
    const tryFn = async () => {
        const str = await fs.readFile(IPCPATH, 'utf8')
        const json: Record<string, any> = JSON.parse(str)
        let value = json[key]
        if (value === undefined) value = null
        return value
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

function logDebugIf(scope:string):boolean {
    const scopes = process.env.CHATNET_DEBUG || ''
    const scopeList = scopes.split(',')
    if (scopeList.includes(scope)) return true
    return false
}

/** runtime debug logs */
function logDebug(...any):void {
    if (!process.env.CHATNET_DEBUG) return

    let result = ''
    for (let each of any) result += inspect(each) + ' '
    if (any.length) result.slice(0, result.length - 1)

    const d = new Date().toISOString()
    const str = '[sio-client]  ' + d + '  ' + result + '\n'
    fs.appendFileSync(IPCLOGF, str)
}

/** checks if event loop should continue running */
async function toLoop() {
    let result = true
	const userstate = await ipcExec(() => ipcGet('userstate'))
	if (userstate === false) result = false

    const now = Date.now()
    await ipcExec(()=>ipcPut('lastping-sioclient', String(Date.now())))
    const lastping = parseInt(await ipcExec(() => ipcGet('lastping-cclient')))
    if ((now-lastping) > 10e3) result = false // cclient is probably dead 💀

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
async function retryableRun(tryFn, catchFn: Function = () => {}) {
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
type SioMessage = { type: 'message'; username: string; data: any}
type SioMessageSend = {type:string, auth: string, data:any}
type SioMeta = { type: 'file'; name: string; data: string }
