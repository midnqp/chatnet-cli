import commander from 'commander'
import handlers from '@src/handlers/index.js'

const program = new commander.Command()

program
    .name('chatnet-cli')
    .description('an experimental chat application for the terminal')
    .helpOption('--help')
    .action(() => {handlers.chat.action()})

program.parse()