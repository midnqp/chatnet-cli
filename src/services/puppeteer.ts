import puppeteer, { EvaluateFunc, Page, Puppeteer } from 'puppeteer'
import services from '@src/services/index.js'

class ChatnetPuppeteerService {
    constructor() { }

    private page?:Page

    private notInitializedError = Error('Service not initiailized. Did you run `init()`?')

    async init(): Promise<Page> {
        if (this.page) return this.page

        const browser = await puppeteer.launch({
            headless: 'new',
            ignoreDefaultArgs: [ "--mute-audio"],
            args: ["--autoplay-policy=no-user-gesture-required", '--use-fake-ui-for-media-stream'],
        })
        services.logger.info('puppeteer executable file:', browser.process()?.spawnfile)

        this.page = await browser.newPage()
        
        this.page.on('console', (event) => {
            services.logger.info('puppeteer output: '+ event.text())
        })
        
        this.page.on('error', event => {
            services.logger.info('puppeteer output: '+ event)
        })

        return this.page
    }

    goto(url:string) {
        if (!this.page) throw this.notInitializedError
        return this.page.goto(url)
    }

    onPageMessage(func:(data:any)=>void) {
        if (!this.page) throw this.notInitializedError
        return this.page.on('message', func)
    }

    async eval(funcString:EvaluateFunc<[]>|string) {
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