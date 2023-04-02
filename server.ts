import {Server} from 'socket.io'
import Database from "@replit/database"
import _http from 'node:http'

const http = _http.createServer((req, res) => { res.end() });
const io  = new Server(http, {
	cors: {
		origin: '*'
	}
});

io.on('connection', (socket) => {
	console.log('a user connected');

	socket.on('register', (msg) => {
		// TODO: check in database if username is taken by someone at this moment
		// JSON.parse(msg).username	
	})

	socket.on('message', (msgdata) => {
		console.log('a new message:', msgdata)
		// TODO: JSON.parse(msg).username
		// TODO: check in database if username available
		try { 
			//const parsed = JSON.parse(msgdata)
			io.emit('broadcast', msgdata);
		} catch(err) {}
	});

	socket.on('disconnect', () => {
		console.log('user disconnected');
		// TODO: remove username from the list of taken-usernames
		// because i don't its possible for anyone to leave the server
		// without "disconnect"-ing.
	});
});


http.listen(443, () => {
	console.log('server is up');
});

const db = new Database()
