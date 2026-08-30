# libuv-file-sender-protocol

### A client and server following a simple fixed bytes protocol for sending files over the network, made with libuv

This project is compatible with Linux distributions and Windows, the binaries for both Windows and Linux are available in the ``Releases`` section of this Github repository.

## Usage:

1. You run the server on one device and the client on another device (although using it with localhost (127.0.0.1) also works).
Make sure to port forward in case you want a client from outside your network to connect to your server. 
If both the server and the client are within your LAN then you don't need to port forward.
2. You choose one of the 2 modes, the 1st mode is sending a file to the server and the 2nd mode is requesting a file from the server
3. You enter the name/path of the file you want to send or request and the server will either receive the file you sent via the client in raw bytes and then make it a file in its own directory (mode 1) or the server will send the client the raw bytes of the file it requested and the client will make it a file in its own directory (mode 2).


## How to build:
Dependenices: libuv and cjson

### Linux Distros:

1. ``mkdir build && cd build``
2. `` cmake ..``
3. ``make``

### Windows
You can build it either with MinGW inside MSYS2 with make or ninja using the MSYS2 UCRT64 environment, Clang with Ninja or you can directly build it with cl.exe and the Visual Studio Generator (which cmake also supports).

Note: When building it outside of MinGW, even if you build it with clang, you still need to install Visual Studio and install the ``C++ desktop development package`` (such a suitable name for a C project prequisite!! Thanks Windows!) or otherwise you'll get some weird errors that you're missing some things.

I'm not going to provide a step by step tutorial on how to build it on Windows because it's beyond my pay grade (which is exactly $0 :D), but I'm going to tell you what I did when I built it in the most straightforward way I found (ps: I built it inside a VM on Windows 10):

1. I installed Visual Studio + the ``C++ desktop development package`` from it, then I installed CMake and VS Codium
2. I made a directory called ``build`` with ``mkdir build``
3. I did ``cd build`` and then ``cmake ..`` followed by ``make``
4. I looked for the DLL files of libuv and cjson that got built and put them all in the same directory with the 2 executables
5. I put the config.json file in the directory and then ran the executables


