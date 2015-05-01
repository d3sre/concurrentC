//
// Created by des on 5/1/15.
//

#ifndef GRABFIRST_GRABPROTOCOLTRANSLATOR_H
#define GRABFIRST_GRABPROTOCOLTRANSLATOR_H

enum command {HELLO, SIZE, NACK, START, TAKE, TAKEN, INUSE, STATUS, END, PLAYERNAME, ERROR};

struct action {
    enum command cmd;
    int iParam1;
    int iParam2;
    char sParam1[256];
};

void decode(const char* cmd, struct action* a);
void encode(struct action* a, char* returnMessage);

#endif //GRABFIRST_GRABPROTOCOLTRANSLATOR_H
