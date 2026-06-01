# Messaging Application in C/C++
## A simple client-server messaging application for use over a subnet
>socket and selector must be compiled beforehand
>compile the server and client
>run the server with `chatserver <listening-port>`
>run the clients with `chatclient <server-addr> <portnum> (host or guest) <sess-id>`
>message sizes are limited to 128 bytes including newlines

## Future work
>encryption, likely using DH key exchange and AES
