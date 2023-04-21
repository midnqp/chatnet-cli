<p align=center><img src="https://user-images.githubusercontent.com/50658760/233596852-935b4351-12c8-4cb1-91b0-ddd2b0e5bcee.png"></p>

This is an experimental text chat client application for the terminal. Binary executables are available for Linux in the **Release** page. The server code is written in Node.js and available [here](https://replit.com/@midnqp/chatnet-server) at Replit. As of now, this is a minimalist implementation and clients connect to a global Replit-hosted network. Work is underway to add features such as markdown text messages, voice clips, image, file share, and voice call.

<p align=center> <img src="https://user-images.githubusercontent.com/50658760/233672086-88d9e1e6-a6fc-4ffd-924d-5ed1701cf0af.GIF"></p>


### What's Unique ✨
There's been always an effort in this project to not use [ncurses](https://invisible-island.net/ncurses/announce.html). Rather explore possibilities of cmdline applications without ncurses-like interfaces that cover up the entire terminal screen. This idea is a challenge to implement, which could be why there are exceptionally few terminal interfaces that take stdin input while printing to stdout at the exact same time.
