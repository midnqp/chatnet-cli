import os from "os";
import socketioclient from "socket.io-client";
import levelup from "levelup";
import leveldown from "leveldown";
import fs from "fs-extra";
main();

async function main() {
	console.log = () => {}

  const ENV = process.env;
  const serverurl = "https://chatnet-server.midnqp.repl.co";
  const io = socketioclient(serverurl);
  //const db = new level.Level(await getDBpath());
  const db = levelup(leveldown(await getDBpath()));
  await db.put("recvmsgbucket", "[]");
  await db.put("sendmsgbucket", "[]");
  await db.put("userstate", "true");

  type DbMsgBucket = Array<SioMessage>;
  type SioMessage = { type: "message"; username: string; data: string };
  type SioMeta = { type: "file"; name: string; data: string };
  // TODO upload file, and delete after 5 min, regardless anything :)
  // TODO voice message ;) too much idea

  io.on("broadcast", (msg: SioMessage) => {
    let tmp: any = {};
    console.log("got broadcast", msg);
    db.get("recvmsgbucket")
      .then(async (recvmsgbucket: any) => {
        tmp.recvbucketstr = recvmsgbucket.toString();
        const arr: DbMsgBucket = JSON.parse(recvmsgbucket.toString());
        arr.push(msg);
        await db.put("recvmsgbucket", JSON.stringify(arr));
      })
      .catch((err) => {
        tmp.err = err;
        console.log("db.get recvmsg failed", tmp);
      });
  });

  setTimeout(() => {
    console.log("set userstate to false");
    db.put("userstate", "false");
  }, 20000);

  while (await toLoop()) {
    let tmp: any = {};
    try {
      const sendmsgitems = (await db.get("sendmsgbucket")).toString();
      tmp.sendmsgitemstr = sendmsgitems;
      let arr: DbMsgBucket = JSON.parse(sendmsgitems);
      await db.put("sendmsgbucket", "[]");
      for (let item of arr) {
        if (item.type == "message") io.emit("message", item);
      }
    } catch (err) {
      tmp.err = err;
      console.log("db.get sendmsg failed", tmp);
    }

    console.log((await db.get("recvmsgbucket")).toString());

    await sleep(1 * 1000);
  }

  io.close();

  /** checks if event loop should continue running */
  async function toLoop() {
    try {
      const userstate = (await db.get("userstate")).toString();
      console.log({ userstate });
      if (userstate == "true") return true;
      else return false;
    } catch (err) {
      return false;
    }
  }

  function sleep(ms: number) {
    return new Promise((r) => setTimeout(r, ms));
  }

  async function getDBpath() {
    let path = "";
    if (os.platform() == "linux") {
      path = ENV.HOME + "/.config/chatnet-client";
      await fs.ensureDir(path);
    }
    return path;
  }
}
