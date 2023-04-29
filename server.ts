import { Server } from 'socket.io'
import http from 'node:http'
import express from 'express'
import cors from 'cors'
import _ReplitDb, { Client } from '@replit/database'

// @ts-ignore
const Replit: typeof Client = new _ReplitDb()
const replitDb = new Replit()
const app = express()
await setupExpressApis(app, replitDb)
const server = http.createServer(app)
const io = new Server(server, { cors: { origin: '*' } })
const LATEST_VERSION = '2.0.1'
const UNSUPPORTED_VERSIONS = ['']
const connectedUsers = new Set()

io.on('connection', socket => {
    const version = socket.handshake.query['version']
    if (typeof version != 'string') {
        socket.emit('message', {
            username: 'chatnet',
            type: 'message',
            data: 'Version not specified, Chatnet will exit now. To download the latest version, visit https://github.com/MidnQP/chatnet-cli.',
        })
        socket.disconnect()
        return
    }

    if (version != LATEST_VERSION) {
        socket.emit('message', {
            username: 'chatnet',
            type: 'message',
            data: latestVersionMsg(LATEST_VERSION, version),
        })
        if (!UNSUPPORTED_VERSIONS.includes(version)) return

        socket.emit('message', {
            username: 'chatnet',
            type: 'message',
            data: 'The version you are running is not supported. Chatnet will exit now.',
        })
        socket.disconnect()
    }

    connectedUsers.add(socket)
    console.log('a user connected of', 'v' + version)

    socket.on('message', (msgdata, ack) => {
        console.log('a new message', msgdata)
        socket.broadcast.emit('broadcast', msgdata)
        ack()
    })

    socket.on('disconnect', () => {
        connectedUsers.delete(socket)
        console.log('user disconnected')
    })
})

server.listen(3000, () => {
    console.log('server is up')
})

function latestVersionMsg(latest, current) {
    return `Latest version is *v${latest}*, however you are running *v${current}*. The latest version includes the newest features and security updates. To download, visit https://github.com/MidnQP/chatnet-cli.`
}

async function setupExpressApis(app: express.Express, replitDb: Client) {
    const users = await replitDb.get('users')
    if (!users) {
        console.log('replitdb init set: users {}')
        await replitDb.set('users', {})
    }

    app.use(cors())
    app.use(express.json())

    app.get('/voice/peers/:username', async (req, res) => {
        const { username } = req.params

        const users = (await replitDb.get('users')) as Record<string, any>
        const result = users[username]

        res.json(result)
    })
    app.post('/voice/peers/:username', async (req, res) => {
        const { username } = req.params
        const { peerId } = req.body
        const result = { ok: true }

        const users = (await replitDb.get('users')) as Record<string, any>
        if (!users[username]) users[username] = {}
        users[username]['peerId'] = peerId
        await replitDb.set('users', users)

        res.json(result)
    })
}

type SioMessage = { type: 'message'; username: string; data: any }
