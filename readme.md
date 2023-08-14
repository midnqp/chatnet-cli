<p align=center><img src="https://user-images.githubusercontent.com/50658760/234240342-e728c354-c168-4a39-a574-7df2e5cabe00.png"></p>

This is an experimental text chat client application for the terminal. Binary executables are available for Linux in the **Release** page. The server code is written in Node.js and available [here](https://replit.com/@midnqp/chatnet-server) at Replit. As of now, this is a minimalist implementation and clients connect to a global Replit-hosted network. Work is underway to add features such as markdown text messages, voice clips, image, file share, and voice call.

<p align=center> <img src="https://user-images.githubusercontent.com/50658760/233672086-88d9e1e6-a6fc-4ffd-924d-5ed1701cf0af.GIF"></p>


### What's Unique ✨
There's been always an effort in this project to not use [ncurses](https://invisible-island.net/ncurses/announce.html). Rather explore possibilities of cmdline applications without such interfaces that cover up the entire terminal screen. This idea is a challenge to implement, which could be why there are exceptionally few terminal interfaces that take stdin input while printing to stdout at the exact same time.

### Build
Pretty straightforward. Make sure to have CMake and dev deps installed.
```sh
sudo apt install libjson-c-dev uuid-dev libgc-dev libcmark-dev
npm install
mkdir dist
npm run build:bin
cd dist && cmake .. && make && cd ..
# optional
sudo mv dist/chatnet dist/chatnet-sio-client  /usr/local/bin/
```

For static binaries, simply replace the 5th line with:
```sh
cd dist && cmake -DCMAKE_BUILD_TYPE=Release .. && make && cd ..
```

To build and run on Windows, make sure to use [WSL](https://learn.microsoft.com/en-us/windows/wsl/install).
