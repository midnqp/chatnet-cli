import util from 'util'
import os from 'os'
import socketioclient from 'socket.io-client'
import fs from 'fs-extra'
const serverurl = 'https://chatnet-server.midnqp.repl.co'
const io = socketioclient(serverurl)
const dbtimeoutMs = 15000 as const
let dbdir = ''
let dbpath = ''

main()

async function main() {
    dbdir = await getDBdir()
    dbpath = getDBpath()
    logdebug('hi')

    try {
        // TODO upload file, and delete after 5 min, regardless anything :)
        // TODO voice message ;) too much idea

        logdebug("listening for 'broadcast'")
        io.on('broadcast', (msg: SioMessage) => {
            dbDo(() => dbGet('recvmsgbucket')).then(
                async (recvmsgbucket: any) => {
                    const arr: DbMsgBucket = JSON.parse(recvmsgbucket)
                    //arr.push(JSON.parse(msg))
                    if (Array.isArray(arr)) {
                        arr.push(msg)
                        const arrstr = JSON.stringify(arr)
                        await dbDo(() => dbPut('recvmsgbucket', arrstr))
                    }
                }
            )
        })

        logdebug('send-msg-loop starting')
        while (await toLoop()) {
            try {
                const sendbucket = await dbDo(() => dbGet('sendmsgbucket'))
                let sendbucketarr: DbMsgBucket = JSON.parse(sendbucket)

                if (!sendbucketarr.length) continue
                await dbDo(() => dbPut('sendmsgbucket', '[]'))
                logdebug('found sendmsgbucket', sendbucketarr)
                for (let item of sendbucketarr) {
                    if (item.type == 'message') {
                        logdebug('sending message', item)
                        io.emit('message', item)
                    }
                }
            } catch (err) {
                logdebug('something went wrong', err)
                throw err
            }

            await sleep(0.5 * 1000)
        }
        logdebug('loop ended, closing socket.io')
        io.close()
    } catch (err) {
        logdebug('something went very wrong', err)
        throw err
    }
    logdebug('bye')
}

async function dbGet(key) {
	const dbstr = await fs.readFile(dbpath, 'utf8')
	logdebug(`dbGet: db.json: `, dbstr)
	const dbjson = JSON.parse(dbstr)
    //let dbjson:any = await fs.readFile(dbpath)
    //logdebug(`dbGet: key=${key} db=${JSON.stringify(dbjson)}`)
    let value = dbjson[key]
    if (value === undefined) value = null
    //logdebug(`................. value=${value}`)
    return value
}

async function dbPut(key, val) {
	const dbstr = await fs.readFile(dbpath, 'utf8')
	logdebug(`dbPut: db.json: `, dbstr)
	const dbjson = JSON.parse(dbstr)
    //const dbjson = await fs.readJson(dbpath)
    //logdebug(`dbPut: key=${key} value=${val} db=${JSON.stringify(dbjson)}`)
    dbjson[key] = val
    await fs.writeJson(dbpath, dbjson)
    //const contents = JSON.stringify(dbjson)
    //await fs.writeFile(dbpath, contents)
}

function logdebug(...any) {
    if (process.env.CHATNET_DEBUG) {
        let result = ''
        for (let each of any) result += util.inspect(each) + ' '
        if (any.length) result.slice(0, result.length - 1) // extra whitespace

        const d = new Date().toISOString()
        const str = '[sio-client]  ' + d + '  ' + result + '\n'
        fs.appendFileSync(getlogfile(), str)
    }
}

/** checks if event loop should continue running */
async function toLoop() {
    let result = true
    try {
        const userstate = await dbDo(() => dbGet('userstate'))
        if (userstate == 'false') result = false
        //else result = false
    } catch (err) {
		//result = false
		result=true
	}
    return result
}

function sleep(ms: number) {
    return new Promise(r => setTimeout(r, ms))
}

async function getDBdir() {
    let path = ''
    if (os.platform() == 'linux') {
        path = process.env.HOME + '/.config/chatnet-client'
        await fs.ensureDir(path)
    }
    return path
}

function getDBpath() {
    return dbdir + '/db.json'
}

function getDBlockfile() {
    return dbdir + '/LOCK'
}

function getDBunlockfile() {
    return dbdir + '/UNLOCK'
}

function getlogfile() {
    return dbdir + '/log-latest.txt'
}

async function setDBlock() {
    while (true) {
        try {
            await fs.rename(getDBunlockfile(), getDBlockfile())
            //logdebug('success: db locked')
            break
        } catch (err) {
            //logdebug('failed setting lock, retyring')
            await sleep(50)
        }
    }
}

async function unsetDBlock() {
    while (true) {
        try {
            await fs.rename(getDBlockfile(), getDBunlockfile())
            //logdebug('success: db unlocked')
            break
        } catch (err) {
            //logdebug('failed unsetting lock, retyring')
            await sleep(50)
        }
    }
}

async function dbDo(fn: (db) => Promise<any>) {
    let ms = 0
    const lockfile = getDBlockfile()
    const unlockfile = getDBunlockfile()
    let result: any
    let fnExecuted = false

    while (ms < dbtimeoutMs) {
        const existsLock = await fs
            .access(lockfile)
            .then(_ => true)
            .catch(_ => false)
        const existsUnlock = await fs
            .access(unlockfile)
            .then(_ => true)
            .catch(_ => false)
        if (!existsLock && existsUnlock) {
            await setDBlock()
            result = await fn(undefined)
            fnExecuted = true
            await unsetDBlock()
            break
        }

        logdebug(`database locked, retrying ... (${ms}/${dbtimeoutMs}) ms`)
        const waitMs = 100 // 500 -> 100
        ms += waitMs
        await sleep(waitMs)
    }
    if (fnExecuted == false) throw Error('database not reachable')
    return result
}

type DbMsgBucket = Array<SioMessage>
type SioMessage = { type: 'message'; username: string; data: string }
type SioMeta = { type: 'file'; name: string; data: string }
