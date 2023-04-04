import os from 'os'
import socketioclient from 'socket.io-client'
import fs from 'fs-extra'
const serverurl = 'https://chatnet-server.midnqp.repl.co'
const io = socketioclient(serverurl)
const dbtimeoutMs = 5000 as const // 5000 ms = 5 sec
let dbdir = ''
let dbpath = ''
main()

async function main() {
    dbdir = await getDBdir()
    dbpath = getDBpath()

    try {
        // TODO upload file, and delete after 5 min, regardless anything :)
        // TODO voice message ;) too much idea

        io.on('broadcast', (msg: SioMessage) => {
            let tmp: any = {}
            dbDo(db => dbGet('recvmsgbucket'))
                .then(async (recvmsgbucket: any) => {
                    tmp.recvbucketstr = recvmsgbucket.toString()
                    const arr: DbMsgBucket = JSON.parse(
                        recvmsgbucket.toString()
                    )
                    arr.push(msg)
                    await dbDo(db =>
                        dbPut('recvmsgbucket', JSON.stringify(arr))
                    )
                })
                .catch(err => {
                    tmp.err = err
                })
        })

        while (await toLoop()) {
            try {
                const sendmsgbucketb = await dbDo(db => dbGet('sendmsgbucket'))
                const sendmsgbucket = sendmsgbucketb.toString()

                let sendmsgarr: DbMsgBucket = JSON.parse(sendmsgbucket)
                await dbDo(db => dbPut('sendmsgbucket', '[]'))
                for (let item of sendmsgarr) {
                    item.type == 'message' &&
                        io.emit('message', JSON.stringify(item))
                }
            } catch (err) {
                logdebug('something went wrong', err)
				throw err
            }

            await sleep(1 * 1000)
        }

        io.close()
    } catch (err) {
        logdebug('something went very wrong', err)
		throw err
    }
}

async function dbGet(key) {
    const dbstr = await fs.readFile(dbpath, 'utf8')
    const dbjson = JSON.parse(dbstr)
    const value = dbjson[key]
    if (value === undefined) return null
    return value
}

async function dbPut(key, val) {
    const dbstr = await fs.readFile(dbpath, 'utf8')
    const dbjson = JSON.parse(dbstr)
    dbjson[key] = val
    const contents = JSON.stringify(dbjson)
    await fs.writeFile(dbpath, contents)
}

function logdebug(...any) {
    if (process.env.DEBUG) console.log(...any)
}

/** checks if event loop should continue running */
async function toLoop() {
    try {
        const userstate = (await dbDo(db => dbGet('userstate'))).toString()
        logdebug({ userstate })
        if (userstate == 'true') return true
        else return false
    } catch (err) {
        return false
    }
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

function setDBlock() {
    return fs.rename(getDBunlockfile(), getDBlockfile())
}

function unsetDBlock() {
    return fs.rename(getDBlockfile(), getDBunlockfile())
}

async function dbDo(fn: (db) => Promise<any>) {
    let ms = 0
    const lockfile = getDBlockfile()
    let result: any
	let fnExecuted = false

    while (ms < dbtimeoutMs) {
        if (!(await fs.exists(lockfile))) {
            await setDBlock()
            result = await fn(undefined)
			fnExecuted = true
            await unsetDBlock()
            break
        }

        logdebug(`database locked, retrying ... (${ms}/${dbtimeoutMs}) ms`)
        const waitMs = 500
        ms += waitMs
        await sleep(waitMs)
    }
	if (fnExecuted == false) throw Error('database not reachable')
    return result
}

type DbMsgBucket = Array<SioMessage>
type SioMessage = { type: 'message'; username: string; data: string }
type SioMeta = { type: 'file'; name: string; data: string }
