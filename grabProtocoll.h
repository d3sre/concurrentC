//
// Created by des on 4/26/15.
//

#ifndef GRABFIRST_GRABPROTOCOLL_H
#define GRABFIRST_GRABPROTOCOLL_H

#endif //GRABFIRST_GRABPROTOCOLL_H

void HELLO();

void SIZE(int n);

void NACK();

void START() ;

void TAKE(int x, int y, char *playerName);

void TAKEN();

void INUSE();

void STATUS(int x, int y);

void PLAYERNAME();

void END(char *playerName);