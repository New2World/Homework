# Computer Network Lab 1 - P2P System

## System

#### Client

Client keep local files information, including file name, file size, chunks of each file, and a name list of files sharing in the system. Client will work in following steps.

1. Setup connection to server, and tell the server which port will be used for sharing;
2. Start a thread for sharing;
3. Get command from user:
    - `reg` : register all files to be shared at server;
    - `ls`  : list all files that are being shared in the P2P system;
    - `loc` : retrieve location information about a certain file;
    - `get` : download a file from all peers holding it;
    - `getr`: download a file from all peers holding it using 'rarest first' policy;
    - `exit`: tell the server to remove all information about current client, and leave the system;

#### Server

The most important job for server is to maintain client and file information. All clients will be managed in a hash map, including clients' IP, ports, and file list. Server will keep a meta information table for all files, maintaining file name, file size, number of chunks, number of clients that is holding the file, and bitmap for chunks. Server will work in following steps.

1. setup a socket and wait for client connections at port `12000`;
2. start a new thread for each client connection, and deal with requests from that client;

<div STYLE="page-break-after: always;"></div>

## Protocol

#### File Download

```
+--------------------------+--------------------------+
|     Client (Download)    |     Client (Upload)      |
+--------------------------+--------------------------+
| Send:                    |                          |
|   1. request type: 6     |                          |
|   2. file name           |                          |
+--------------------------+--------------------------+ --+
| Send:                    |                          |   |
|   1. chunk ID            |                          |   |
+--------------------------+--------------------------+   |
|                          | Send:                    |   |    repeat until
|                          |   1. chunk ID            |   |--> finishing
|                          |   2. chunk content       |   |    transmission
|                          |   2. content length      |   |
+--------------------------+--------------------------+   |
| Send:                    |                          |   |
|   1. acknowledgement     |                          |   |
+--------------------------+--------------------------+ --+
| Send:                    |                          |
|   1. a flag denoting     |                          |
|      the end of          |                          |
|      transmission        |                          |
|                          |                          |
+--------------------------+--------------------------+
```

#### Register Request

```
+--------------------------+--------------------------+
|         Client           |          Server          |
+--------------------------+--------------------------+
| Send:                    |                          |
|   1. request type: 1     |                          |
|   2. number of files: n  |                          |
+--------------------------+--------------------------+
| Send: (n times)          |                          |
|   1. file name           |                          |
|   2. file size           |                          |
|   3. port for sharing    |                          |
+--------------------------+--------------------------+
|                          | Send:                    |
|                          |   1. response type: 1    |
+--------------------------+--------------------------+
```

<div STYLE="page-break-after: always;"></div>

#### File List Request

```
+--------------------------+--------------------------+
|         Client           |          Server          |
+--------------------------+--------------------------+
| Send:                    |                          |
|   1. request type: 2     |                          |
+--------------------------+--------------------------+
|                          | Send:                    |
|                          |   1. response type: 2    |
|                          |   2. number of files: n  |
+--------------------------+--------------------------+
|                          | Send: (n times)          |
|                          |   1. file name           |
|                          |   2. file size           |
+--------------------------+--------------------------+
```

#### File Locations Request

```
+--------------------------+--------------------------+
|         Client           |          Server          |
+--------------------------+--------------------------+
| Send:                    |                          |
|   1. request type: 3     |                          |
+--------------------------+--------------------------+
|                          | Send:                    |
|                          |   1. response type: 3    |
|                          |   2. number of peers: n  |
+--------------------------+--------------------------+
|                          | Send: (n times)          |
|                          |   1. peer's IP           |
|                          |   2. peer's port         |
|                          |   3. number of chunks    |
|                          |   2. bitmap for chunks   |
+--------------------------+--------------------------+
```

#### Chunk Register Request

```
+--------------------------+--------------------------+
|         Client           |          Server          |
+--------------------------+--------------------------+
| Send:                    |                          |
|   1. request type: 4     |                          |
|   2. peer's IP           |                          |
|   2. file name           |                          |
|   2. chunk ID            |                          |
+--------------------------+--------------------------+
|                          | Send:                    |
|                          |   1. response type: 4    |
+--------------------------+--------------------------+
```

#### Leave Request

```
+--------------------------+--------------------------+
|         Client           |          Server          |
+--------------------------+--------------------------+
| Send:                    |                          |
|   1. request type: 5     |                          |
+--------------------------+--------------------------+
|                          | Send:                    |
|                          |   1. response type: 5    |
+--------------------------+--------------------------+
```

## Usage

Compile the code using `Makefile`, notice that the developing environment use compiler `g++-7`.

```
$ make
```

If only client/server is needed, please use

```
$ make client
$ make server
```

#### Server

Start the server with

```
$ ./server
```

#### Client

Before using the client, please prepare a directory and put all files into that directory. Any sub-directory will not be considered to be sharing. To start the client, server's IP and an idle port is needed.

```
$ ./client  <server ip>  <directory path>  <port for sharing>
```

After starting the client, it will show a brief instruction. I assume each user know nothing about files that are being shared when they first login, so a list request (`ls`) must be sent before location (`loc`) or download (`get` or `getr`) operations. And file register request (`reg`) will not be sent automatically, which means that no file will be shared by a client before manually sending register request (`reg`).

<div STYLE="page-break-after: always;"></div>

## Limitations

1. Client cannot find peers behind NAT;
    - The reason is that clients can only find a peer via the IP and the port provided by server.
2. Different files sharing the same name is forbidden;