import {Command} from 'commander'
const program = new Command()

program.name('chatnet-cli').description('an experimental chat application for the terminal, that leverages a modern asynchronous multiplexing approach to stdin and stdout.').helpOption('--help')

program.parse()