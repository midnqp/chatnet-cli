import commander from 'commander'
import handlers from '@src/handlers/index.js'

const program = new commander.Command()

program
.name('chatnet-cli')
.description('an experimental chat application for the terminal')
.helpOption('--help')
.action(catchE(handlers.chat))

try {
    program.parse()
} catch (e) {
    console.error(e)
}

function catchE(fn:Function) {
    const cb = (err:any) => console.error(err)
    return (...args:any) => fn(...args)?.catch(cb)
}

process.on('unhandledRejection', console.error)
process.on('uncaughtException', console.error)