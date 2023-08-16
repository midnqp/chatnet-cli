// https://replit.com/@midnqp/chatnet-server
import { Server } from 'socket.io'
import http from 'node:http'
import crypto from 'node:crypto'
import express from 'express'
import cors from 'cors'
import ReplitDb from '@replit/database'
import jwt from 'jsonwebtoken'
import lodash from 'lodash'
import { z } from 'zod'

/** @type {ReplitDb.Client} */
const replitDb = new ReplitDb()
const app = express()
//await setupExpressApis(app, replitDb)
const server = http.createServer(app)
const io = new Server(server, { cors: { origin: '*' } })
const LATEST = "2.0.1"

const replitDbKeys = await replitDb.list()
if (!replitDbKeys.includes('userauth')) await replitDb.set('userauth', []) // Array<{id:string, username:string, iat:number}>
if (!replitDbKeys.includes('history')) await replitDb.set('history', []) // Array<{username: string, type: string, data: any}>
await replitDb.set('useractive', []) // Array<{uid:string}>


io.on('connection', async socket => {
	// the first contact of client to this server 👈
	let AUTHRESULT

	// view message history
	//getMessageHistory().then(msgHistory => socket.emit('history', msgHistory))

	const handshake = socket.handshake
	console.log('a user connected')
	if (handshake?.auth?.auth) {
		const auth = handshake.auth.auth
		const result = authCheck(auth)
		if (result === false) { }
		else {
			// This user is authorised, logging in 📈, #user-retention 🎉
			AUTHRESULT = result




			// notify about an active user and total count + view message history
			const userActiveList = await replitDb.get('useractive')
			const ua = userActiveList.find(u => u.uid == AUTHRESULT.id)
			const min5 = 300000 // 5 minutes in ms

			if (ua === undefined) { // newly active user
				userActiveList.push({ uid: AUTHRESULT.id, lastActive: Date.now(), isActive: true })
				getMessageHistory().then(msgHistory => {
					console.log({ msgHistory })
					socket.emit('history', msgHistory)
				})
			}
			if ((Date.now() - ua?.lastActive) >= min5) { // only give the message history again, if the user was last active in more than 5 minutes
				getMessageHistory().then(msgHistory => {
					console.log({ msgHistory })
					socket.emit('history', msgHistory)
				})
			}



			const userActiveCount = userActiveList.filter(u => u.isActive).length
			io.emit('broadcast', buildMessage({
				username: 'chatnet',
				type: 'message',
				data: `*${AUTHRESULT.username}* just became active` + ((userActiveCount <= 1) ? ', only you are active 😐' : `, total ${userActiveCount} active`)
			}))
			await replitDb.set('useractive', userActiveList)
		}
	}

	//	socket.on('voice:in', chunk => socket.broadcast.emit('voice:out', chunk))

	socket.on('fileshare', (msgdata, ack) => { })

	socket.on('voicemessage', (msgdata, ack) => {
		const authresult = authCheck(msgdata.auth)
		if (authresult === false) {
			socket.emit('broadcast', buildMessage({
				username: 'chatnet',
				type: 'message',
				data: 'auth failed'
			}))
			ack()
			return
		}

		console.log('well well well, a voice message, is it?', msgdata.length)
		const msgToEmit = buildMessage({
			type: 'voicemessage',
			username: authresult.username,
			data: msgdata.data
		})
		socket.broadcast.emit('voicemessage', msgToEmit)
		addToMessageHistory(msgToEmit)
		ack()
	})

	socket.on('auth', async (msgdata, ack) => {
		// signup or change-req?
		const { auth, type, data } = msgdata
		let newUsername = msgdata.data
		let oldUsername = ''
		if (auth == '') {
			oldUsername = ''
		}
		else {
			const authresult = authCheck(msgdata.auth)
			if (authresult === false) return ack({ auth: '' })
			oldUsername = authresult.username
		}
		console.log('username change request', oldUsername, '->', newUsername)

		// username validation
		const usernameSchema = z.string().max(16).min(4).trim().toLowerCase().refine(s => s.match(/[a-z]/g).join("") === s)
		const validation = usernameSchema.safeParse(newUsername)
		const constraintsMsg = `username constraints are:\r\n` + `- max 16 and min 4 letters\r\n` + `- alphabetic letters only`
		if (validation.success === false) {
			socket.emit('broadcast', buildMessage({ username: 'chatnet', type: 'message', data: constraintsMsg }))
			return ack({ auth: '' })
		}
		else {
			newUsername = validation.data
		}

		// check if already same
		if (oldUsername == newUsername) return ack({ auth })

		// all good, now create bearer token and send!
		const bearer = await authCreate({ username: newUsername })
		if (bearer === false) {
			socket.emit('broadcast', buildMessage({ username: 'chatnet', type: 'message', data: 'sorry, someone with this username is already signed up :(' }))
			return ack({ auth: '' })
		}

		// notify everyone about this existing/new person 🧔
		if (oldUsername != '') {
			// since this user is renaming, free the old username.
			const userauthList = await replitDb.get('userauth')
			const ua = userauthList.find(ua => ua.username == oldUsername)
			lodash.remove(userauthList, ua)
			await replitDb.set('userauth', userauthList)

			io.emit('broadcast', buildMessage({ username: 'chatnet', type: 'message', data: `username changed *${oldUsername}* -> *${newUsername}*` }))
			AUTHRESULT.username = newUsername // update the new username
		}
		else {
			AUTHRESULT = authCheck(bearer)
			io.emit('broadcast', buildMessage({ username: 'chatnet', type: 'message', data: `a new person just signed up: *${newUsername}*` }))
			// treat him like a newly active user
			const userActiveList = await replitDb.get('useractive')
			userActiveList.push({ uid: AUTHRESULT.id, lastActive: Date.now(), isActive: true })
			getMessageHistory().then(msgHistory => {
				console.log({ msgHistory })
				socket.emit('history', msgHistory)
			})
		}
		ack({ auth: bearer })
	})

	socket.on('message', (msgdata, ack) => {
		const authresult = authCheck(msgdata.auth)
		if (authresult === false) {
			socket.emit('broadcast', buildMessage({
				username: 'chatnet',
				type: 'message',
				data: 'auth failed'
			}))
			ack()
			return
		}

		console.log(authresult.username + ':', msgdata.data)
		const msgToEmit = buildMessage({
			username: authresult.username,
			type: msgdata.type,
			data: msgdata.data
		})
		socket.broadcast.emit('broadcast', msgToEmit)
		addToMessageHistory(msgToEmit)
		ack()
	})

	// TODO without some kind of auth usertoken, i can't ever properly maintain an active 
	// list of users. :( probably that's it still isn't implemented.
	socket.on('disconnect', async () => {
		console.log('user disconnected')

		if (AUTHRESULT !== undefined) {
			const activeList = await replitDb.get('useractive')
			const userLeaving = activeList.find(user => user.uid === AUTHRESULT.id)
			//lodash.remove(activeList, userLeaving) // don't remove, update `lastActive`
			userLeaving.lastActive = Date.now()
			userLeaving.isActive = false
			const userActiveCount = activeList.filter(u => u.isActive).length

			io.emit('broadcast', buildMessage({
				username: 'chatnet',
				type: 'message',
				data: `*${AUTHRESULT.username}* just became inactive, total ${userActiveCount} active`
			}))
			await replitDb.set('useractive', activeList)
		}
	})
})

server.listen(3000, () => {
	console.log('server is up')
})

function buildMessage(o) {
	if (o.createdAt === undefined) o.createdAt = String(Date.now()) // because: getMessageHistory() will already have `createdAt` preserved!
	console.log(o)
	return o
}

// TODO store voice messages, perhaps use a C server
// to truncate the audio file to 30 seconds?
async function addToMessageHistory(msg) {
	const history = await replitDb.get("history")
	if (history.length === 495) history.shift()

	if (msg.type == 'voicemessage') {
		msg.data = '🎶 *this was a voice message*'
	}
	if (msg.type == 'fileshare') {
		msg.data = '📁 *this was a file-share*'
	}

	history.push(msg)
	await replitDb.set('history', history)
}

async function getMessageHistory() {
	const history = await replitDb.get('history')
	const result = history.slice(-20)
	return result
}

/** 
 * This does not do a round trip to database. Because this runs per message!
 * @returns {false|object}
 */
function authCheck(bearerToken) {
	try {
		const { username, iat, id } = jwt.decode(bearerToken, process.env.JWT_SECRET);
		return { username, iat, id }
	}
	catch (err) { return false }
}

/** 
 * Checks whether username is unique, then 
 * creates a bearer token, adds the user to db,
 * and returns the token.
 * @returns {false|string}
 */
async function authCreate({ username }) {
	const userauthList = await replitDb.get('userauth')
	const found = userauthList.find(ua => ua.username == username)
	if (found) return false

	const iat = Date.now()
	const id = crypto.randomUUID()
	const bearer = jwt.sign({ username, iat, id }, process.env.JWT_SECRET)
	userauthList.push({ username, iat, id })
	await replitDb.set('userauth', userauthList)

	return bearer
}

/** 
 * TODO This comes much later, after voice message.
 * @param {express.Express} app 
 * @param {ReplitDbClient} replitDb
 */
async function setupExpressApis(app, replitDb) {
	const users = await replitDb.get("users")
	if (!users) {
		console.log('replitdb init set: users {}')
		await replitDb.set("users", {})
	}

	app.use(cors())
	app.use(express.json())

	app.get('/voice/peers/:username', async (req, res) => {
		const { username } = req.params

		const users = await replitDb.get('users')
		const result = users[username]

		res.json(result)
	})
	app.post('/voice/peers/:username', async (req, res) => {
		const { username } = req.params
		const { peerId } = req.body
		const result = { ok: true }

		const users = await replitDb.get('users')
		if (!users[username]) users[username] = {}
		users[username]['peerId'] = peerId
		await replitDb.set('users', users)

		res.json(result)
	})
}

