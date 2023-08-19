## Introduction

The `IX83 Server` is a daemon process that facilitates communication with the Olympus IX83 microscope (IX3-TPC) and provides an API for controlling the microscope through TCP/IP sockets.

The server is designed to be used in conjunction with a custom `IX83 Client` application, which can be developed with any language supporting TCP/IP communication, including Python, MATLAB, LabVIEW, and more. This is especially useful as the Olympus SDK's callback functions can be challenging to implement in languages other than C/C++.

## Getting Started

To build and run, you will need to install [MSVC](https://visualstudio.microsoft.com/downloads/), [Qt](https://www.qt.io/download-open-source) (version greater than 6.5), and the Olympus IX3 SDK.

> Note:
> 
> 1. It is recommended to use the MSVC compiler rather than MinGW, as the Olympus SDK is more compatible with MSVC. Olympus commands are encoded in ASCII, while MinGW might send commands encoded differently, causing compatibility issues with the IX3-TPC.
> 2. Due to the use of some windows APIs, the server may not be compatible with other operating systems. It has only been tested on Windows so far.

To build the project, open the `IX83Server.pro` file with Qt Creator and build the project.

## Request Format

The `IX83 Server` accepts two types of request, both of which must end with the end-of-line `\r\n` character. The server's responses will also end with the end-of-line `\r\n` character.

- To interact with the Daemon, utilize `{Daemon}` as the header.
  The Daemon accepts the following commands:
  - `InitializePortManager`
  - `IsConnected`
  - `EnumerateInterface`
  - `OpenInterfaceX`

    Note that the `X` in the `OpenInterfaceX` command should be replaced with the interface number, starting from 0, after sending the `EnumerateInterface` command.

  - `CloseInterface`
  - `LogIn`
  - `LogOut`
  - `QuitPortManager`
  - `QuitDaemon`
  
  > All commands are case-insensitive and have aliases.
    
- To communicate with the Olympus Port Manager, empoly `[Olympus]` as the header, and follow the Olympus IX83 IF specification.
  - The Olympus Port Manager will forward the message after the `[Olympus]` header directly to the IX3-TPC. For more details, please refer to the Olympus SDK documentation `IX3-BSW Application Note_E.pdf` and `IX3TPCif_E.pdf`.
  - You can also use `[Olympus]L 1,0` command to log in to the IX3-TPC, and `[Olympus]L 0,0` to log out. These commands will have the same effect as `{Daemon}LogIn}` and `{Daemon}LogOut`.

## Arugments

- `-p 1500` : Specifies the port number of the server. 
  - The default port number is `1500`.
  - The Olympus Port Manager will use `port+1` to send active notifactions to the server (in custom build client) that is listening to `port+1`.
- `-e true` : Specifies whether the server is exclusive.
  - The default value is `true`.
  - If the server is exclusive, it will not accept any other connection requests while already connected with a client.
- `-t 5` : Specifies the time interval (in seconds) for the daemon to check the server's status.
  - The default value is `5` seconds.
  - Set this value to `0` to disable the check.
- `-show` : Specifies whether to show the console window.
  - This option does not require any additional arguments.
  - If this argument is omitted, the console window will not be shown.

## Safe Termination

In principle, manual termination of the daemon process is unnecessary. After sending the `{Daemon}QuitPortManager` command, the Olympus Port Manager will automatically close the interface and destroy itself. Subsequently, the microscope can be manually shut down in a safe manner.

However, two termination codes are writen to safely stopping the daemon process in exceptional situations: `KillConsole` and `KillAll` (see the `.pro` file).  Both codes operate similarly, but KillAll will terminate all processes without prompting for confirmation.

## License

This project is licensed under the GNU GPL v3 License. Refer to the `LICENSE` file for details.
