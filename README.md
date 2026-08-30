# libuv-file-sender-protocol

### A client and server following a simple fixed bytes protocol for sending files over the network, made with libuv

This project should be compatible with both Linux distributions and Windows (although, it still hasn't been tested on Windows).

## How to build:
Dependenices: libuv and cjson

### Arch Linux:

1. ``mkdir build && cd build``
2. `` cmake ..``
3. ``make``


## Usage:

1. You run the server on one device and the client on another device (although using it with localhost (127.0.0.1) also works).
Make sure to port forward in case you want a client from outside your network to connect to your server. 
If both the server and the client are within your LAN then you don't need to port forward.
2. You choose one of the 2 modes, the 1st mode is sending a file to the server and the 2nd mode is requesting a file from the server
3. You enter the name/path of the file you want to send or request and the server will either receive the file you sent via the client in raw bytes and then make it a file in its own directory (mode 1) or the server will send the client the raw bytes of the file it requested and the client will make it a file in its own directory (mode 2).
