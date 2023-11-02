import linenoise from '@src/services/linenoise.js'
import stdoutee from '@src/services/stdoutee.js'
import conf from '@src/services/config.js'
import * as common from '@src/services/common.js'
import api from '@src/services/api.js'
import receive from '@src/services/receive.js'
import mic from '@src/services/mic.js'

class ChatnetServices {
    linenoise = linenoise
    stdoutee = stdoutee
    common = common
    config = conf
    api= api
    receive= receive
    mic= mic
}

export default new ChatnetServices()