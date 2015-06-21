# Concurrent C Programming / Seminar ZHAW / FS 2015
Nach den Definitionen aus https://github.com/telmich/zhaw_concurrent_c_programming_fs_2015 

Erzeugung der Files über make

## Start des Servers (Bsp)
    ./server <port> <n> <y> [RELEASE|DEBUG]
./server 54431 5 5 RELEASE

Wobei   
	n = Spielfeldgrösse   -> 4 <= n <= 256  
      	y = Prüfungsinterval  -> 1 <= y <= 30  

## Start des Clients (Bsp)
    ./client <ipaddr> <serverport> [clientname] [gamestrategy]
./client 127.0.0.1 54431 beta 2

Wobei Gamestrategie:
1: simple from 0/0 to n/n - loop  
2: simple quick - loop  
3: random  
4: only STATUS check;  
5: inactive client  


## Testing hints
max tested clients were 262
for i in $(seq 1 260); do (./client 127.0.0.1 54431 $i 5&); done

./client 127.0.0.1 54431 alice 2

./client 127.0.0.1 54431 bob 3
