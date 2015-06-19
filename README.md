# Concurrent C Programming / Seminar ZHAW / FS 2015
Nach den Definitionen aus https://github.com/telmich/zhaw_concurrent_c_programming_fs_2015 

Erzeugung der Files über make

## Start des Servers (Bsp)
    ./server <port> <n> <y> [RELEASE|DEBUG]
./server 54431 5 5 RELEASE

## Start des Clients (Bsp)
    ./client <ipaddr> <serverport> [clientname] [gamestrategy]
./client 127.0.0.1 54431 beta 2

