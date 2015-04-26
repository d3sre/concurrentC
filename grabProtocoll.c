//
// Created by des on 4/26/15.
//
#include <string.h>
#include <stdio.h>
#include "grabProtocoll.h"

void HELLO() {
    printf("HELLO received");
}

// Server only
void SIZE(int n) {
    printf("SIZE is %d", n);
}
// Server only
void NACK() {
    printf("NACK received");
}
// Server only
void START() {
    printf("START received");
}

void TAKE(int x, int y, char *playerName) {
    printf("TAKE received on x:%d y: %d by: %s", x, y, playerName);
}

//Server only
void TAKEN() {
    printf("TAKEN received");
}
//Server only
void INUSE() {
    printf("INUSE received");
}

void STATUS(int x, int y) {
    printf("Status x:d% y:%d",x,y);
}
// Server only
void PLAYERNAME() {
    printf("playername");
}

void END(char *playerName) {
    printf("END player: %s", playerName );
}