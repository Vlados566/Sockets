## Lab 1

server.c contains code for creating socket and waiting for client.c to connect and send text message.  
client.c contains code for connecting to server.c's created socket and send text message.  

## Building
```
git clone https://github.com/PolisanTheEasyNick/LabsSockets
cd LabsSockets/LAB1
mkdir build
cd build
cmake ..
make
```

## Starting
`./server` for server start
`./client` for client start