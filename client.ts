import util from 'util'
import os from 'os'
import socketioclient from 'socket.io-client'
//import rocksdb from 'level-rocksdb'
import rocksdb from 'rocksdb'
import fs from 'fs-extra'
const serverurl = 'https://chatnet-server.midnqp.repl.co'
const io = socketioclient(serverurl)
const dbtimeoutMs = 5000 as const // 5000 ms = 5 sec
const db = rocksdb(getDBpath())

//const dbPut = util.promisify(db.put)
//const dbGet = util.promisify(db.get)
//const dbClose = util.promisify(db.close)
const dbGet = key =>
    new Promise((res, rej) => {
        const cb = function (err, value) {
            if (err) rej(err)
            res(value)
        }
        db.get(key, cb)
    })

const dbPut = (key, val) =>
    new Promise((res, rej) => {
        const cb = function (err) {
            if (err) rej(err)
				res(undefined)
        }
        db.put(key, val, cb)
    })

const dbClose = () =>
    new Promise((res, rej) => {
        const cb = function (err) {
            if (err) rej(err)
			res(undefined)
        }
        db.close(cb)
    })

const dbOpen = () =>
    new Promise((res, rej) => {
        const cb = function (err) {
            if (err) rej(err)
			res(undefined)
        }
        db.open({createIfMissing: true}, cb)
    })

main()

async function main() {
	try {
	logdebug('db connecting')
	await dbOpen()
	logdebug('db connected')

    await dbDo(db => dbPut('recvmsgbucket', '[]'))
    await dbDo(db => dbPut('sendmsgbucket', '[]'))
    await dbDo(db => dbPut('userstate', 'true'))
	logdebug("defaults set")

    // TODO upload file, and delete after 5 min, regardless anything :)
    // TODO voice message ;) too much idea

    io.on('broadcast', (msg: SioMessage) => {
        let tmp: any = {}
        logdebug('got broadcast', msg)
        dbDo(db => dbGet('recvmsgbucket'))
            .then(async (recvmsgbucket: any) => {
                tmp.recvbucketstr = recvmsgbucket.toString()
                const arr: DbMsgBucket = JSON.parse(recvmsgbucket.toString())
                arr.push(msg)
                await dbDo(db => dbPut('recvmsgbucket', JSON.stringify(arr)))
            })
            .catch(err => {
                tmp.err = err
                logdebug('dbGet recvmsg failed', tmp)
            })
    })

    while (await toLoop()) {
        let tmp: any = {}
        try {
            const sendmsgbucketb = await dbDo(db => dbGet('sendmsgbucket'))
            const sendmsgbucket = sendmsgbucketb.toString()
            tmp.sendmsgitemstr = sendmsgbucket

            let sendmsgarr: DbMsgBucket = JSON.parse(sendmsgbucket)
            await dbDo(db => dbPut('sendmsgbucket', '[]'))
            for (let item of sendmsgarr) {
                item.type == 'message' &&
                    io.emit('message', JSON.stringify(item))
            }
        } catch (err) {
            tmp.err = err
            logdebug('in whileloop something failed', tmp)
        }

        const recvmsgbucketb = await dbDo(db => dbGet('recvmsgbucket'))
        logdebug('recvmsgbucket ', recvmsgbucketb.toString())

        await sleep(1 * 1000)
    }

    io.close()
	await dbClose()
	} catch(err) {
		console.log('something went wrong', err)
	}
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

function getDBpath() {
    let path = ''
    if (os.platform() == 'linux') {
        path = process.env.HOME + '/.config/chatnet-client'
        fs.ensureDirSync(path)
    }
    return path
}

function dbDo(fn: (db: rocksdb) => Promise<any>) {
    return fn(db)
}

/*
async function dbDo(func: (db:levelup.LevelUp<LevelDown, AbstractIterator<any, any>>)=>Promise<any>) {
    let timeoutcMs = 0
    let dberr
	let db:any = null
    while (timeoutcMs < dbtimeoutMs) {
        logdebug(`retrying db conn... (${timeoutcMs}/${dbtimeoutMs})`)
        try {
			const dbpath = await getDBpath()
			if (timeoutcMs > 3500) {
				// stayed in queue for 3.5 sec, still no conn?
				// that sounds bad.
				// now... time to remove the LOCK!
				// probably that was due to an unclean exit.
				// also probably the chatnet-c-client might be
				// waiting too. maybe not. possibly might only wait
				// the first time. but it probably starts waiting after
				// this client starts waiting.
				const lockfile = dbpath+'/LOCK'
				if (await fs.exists(lockfile)) {
					await fs.unlink(lockfile)
				}
			}

            const dbconfig = leveldown(dbpath) // this connects to db
            db = levelup(dbconfig)
            break
        } catch (err) {
            dberr = err
            timeoutcMs += 50
            await sleep(50)
        }
    }
    if (!db) throw Error('database conn failed:\n' + dberr.message)
    logdebug('connected to db!')
    logdebug('db executing func: ', func.toString())

    const result = await func(db)
    await dbClose()
    return result
}
*/

type DbMsgBucket = Array<SioMessage>
type SioMessage = { type: 'message'; username: string; data: string }
type SioMeta = { type: 'file'; name: string; data: string }
