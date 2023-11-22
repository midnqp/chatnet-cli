<p align=center><img src="https://user-images.githubusercontent.com/50658760/234240342-e728c354-c168-4a39-a574-7df2e5cabe00.png"></p>

This is an experimental chat application that is designed to be non-blocking. 

Meaning, while writing a message in stdin the app can continue receiving messages and continue printing to stdout simultaneously. At first, this may sound obvious. For command-line applications, however, this isn't the case. Since reading input from the terminal is a synchronous operation, receiving messages ought to run asynchronously or in the background. In that case, if the app prints a received message _"Chatnet released yet?"_ when the user is typing a message to send _"How's been everyth..."_, then the resulting garbled text would be _"How's been everyth<b>Chatnet released yet?</b>ing?"_

This project solves this problem.


<p align=center> <img src="https://github.com/midnqp/chatnet-cli/assets/50658760/dcc5772f-36ef-46a5-b3d5-664853551668"></p>


### What's Unique ✨
To solve the problem, this project follows the async multiplexing approach as developed by [@antirez](https://github.com/antirez) on his [linenoise](https://github.com/antirez/linenoise) project. This allows for a minimal interface without hijacking the entire terminal screen as done by libraries like [ncurses](https://invisible-island.net/ncurses/announce.html). This idea is a challenge to implement, which could be why there are exceptionally few terminal interfaces that take stdin input while printing to stdout at the exact same time.

This approach may serve as an inspiration to build more sophisticated terminal applications that are more engaging and delightful to work with as developers!
