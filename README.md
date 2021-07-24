<p align=center><img src="https://raw.githubusercontent.com/MidnQP/midnqp/midnqp/cdn/terminal-chat-logo-white.PNG"></p>

Join your network of 👨🏻‍💻 people, and be greeted 👋🏻 with peace!

- [Download TerminalChat for Windows](https://github.com/MidnQP/TerminalChat/raw/master/bin/WinTerminalChat_vlatest.zip)
- [Download TerminalChat for Linux/Mac](https://github.com/MidnQP/TerminalChat/raw/master/bin/chatnet-linux-amd64)


# Information
### Building from source
The precompiled binary for Linux/Mac at `/bin/chatnet-linux-amd64` is dynamically linked. To use `chatnet` on Linux/Mac, install these packages using `brew`/`apt`/`pacman`:
```
libunistring-dev libldap-dev libgss-dev libnss3-dev liblber-dev libnspr4-dev libpsl-dev libcurl4-nss-dev librtmp-dev libssh2-1-dev
```
##### For Linux/Mac:

```sh
./make.bash
```

##### For Windows:
```bat
:: You need to use Microsoft Visual Studio.
:: Because:
::     - Mingw doesn't have Address Sanitizer
::     - issue#1

:: Instructions to configure Visual Studio
:: is provided inside make.bat
```


### Setting up your own TerminalChat network
- `chatnet-server.php` is the file to upload at http://yourwebsite.com/path/to/chatnet-server.php
- Inside `lib-chatnet-client.h` change symbol `NETADDR` to http://yourwebsite.com/path/to/chatnet-server.php
- Compile the client implementations and distribute/deploy 🚀!

## TODO 📃
- Store chatlogs locally
- `chatnet send <receiver> <file/folder>`
