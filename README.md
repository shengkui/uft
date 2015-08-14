 Transmit file via datagram socket
====================================
**Author**: Shengkui Leng

**E-mail**: lengshengkui@outlook.com


Description
-----------
This project is a demo for how to transmit file via datagram socket(SOCK_DGRAM).
The client will resend the request if it hasn't received the response.


* * *

Build
-----------
(1) Open a terminal.

(2) chdir to the source code directory.

(3) Run "make"


Run
-----------
(1) Start the server:

>    $ ./server

You can specify the port number with argument:

>    $ ./server -p 7777

Notes:
>    The default port number used by server is 7777.

>    Use '-h' option to get more detail usage information of the server.

(2) Start the client to send file to server:

>    $ ./client test1.txt

You can specify "server ip" and "server port number" with arguments:

>    $ ./client -s 127.0.0.1 -p 7777 test1.txt

Notes:
>    The default server port number is 7777.

>    The default server ip is 127.0.0.1.

>    Use '-h' option to get more detail usage information of the client.

