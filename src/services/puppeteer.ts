import puppeteer, { Page, Puppeteer } from 'puppeteer'
import services from '@src/services/index.js'

class ChatnetPuppeteerService {
    constructor() { }

    private page?:Page

    private notInitializedError = Error('Service not initiailized. Did you run `init()`?')

    async init(): Promise<Page> {
        if (this.page) return this.page

        const browser = await puppeteer.launch({
            headless: 'new',
            ignoreDefaultArgs: ["--mute-audio"],
            args: ["--autoplay-policy=no-user-gesture-required"],
        })
        this.page = await browser.newPage()
        this.page.on('console', (event) => {
            services.stdoutee.print('puppeteer: '+ event.text())
        })

        return this.page
    }

    goto(url:string) {
        if (!this.page)  throw this.notInitializedError
    }

    async eval(funcString:string) {
        if (!this.page) throw this.notInitializedError

        return this.page.evaluate(funcString)
    }

    async close() {
        if (this.page) {
            const page =this.page
            const browser = page.browser()
            delete this.page
            await page.close()
            await browser.close()
        }
    }
}

export default new ChatnetPuppeteerService()