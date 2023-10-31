import { PathLike } from 'node:fs';
import fs from 'node:fs/promises'

export function fileFound(path:string) {
    return fs.access(path).then(() => true).catch(() => false)
}