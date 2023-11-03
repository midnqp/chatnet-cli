import commander from 'commander'
import handlers from '@src/handlers/index.js'

const program = new commander.Command()

program
    .name('chatnet-cli')
    .description('an experimental chat application for the terminal')
    .helpOption('--help')
    .action(handlers.chat.action)

process.on('uncaughtException', err => {
    console.error('chatnet-cli had an unexpected error:')
    console.error(err)

    const cmd = program.args[0] || 'chat'
    if (isCmd(cmd)) handlers[cmd].exit()
    console.log()
})

function isCmd(cmd: string): cmd is keyof typeof handlers {
    return Object.keys(handlers).includes(cmd)
}