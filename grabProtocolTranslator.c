//
// Created by des on 5/1/15.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "grabProtocolTranslator.h"


void decode(const char* cmd, struct action* a) {
    //printf("entered parser");

    char mainCommand[255], proto1st[255], proto2nd[255], proto3rd[255], proto4th[255];


    sscanf(cmd, "%s %s %s %s", proto1st, proto2nd, proto3rd, proto4th);

    if (strcmp(proto1st,"HELLO") == 0) {
        a->cmd = HELLO;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"SIZE") == 0) {
        a->cmd = SIZE;
        a->iParam1 = atoi(proto2nd);
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"NACK") == 0) {
        a->cmd = NACK;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"START") == 0) {
        a->cmd = START;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"TAKE") == 0) {
        a->cmd = TAKE;
        a->iParam1 = atoi(proto2nd);
        a->iParam2 = atoi(proto3rd);
        strcpy(a->sParam1, proto4th);
    }
    else if(strcmp(proto1st,"TAKEN") == 0) {
        a->cmd = TAKEN;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"INUSE") == 0) {
        a->cmd = INUSE;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"STATUS") == 0) {
        a->cmd = STATUS;
        a->iParam1 = atoi(proto2nd);
        a->iParam2 = atoi(proto3rd);
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"END") == 0) {
        a->cmd = END;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, proto2nd);
    }
    /*else if(strcmp(proto1st,"PLAYERNAME") == 0) {
        a->cmd = PLAYERNAME;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, proto1st);
    }*/
    else {
        a->cmd = PLAYERNAME;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, proto1st);
    }


//    if(strcmp(proto1st,"HELLO") == 0){
//        //TODO handle not successful sign up
//        p[0] = SIZE();
//        p[1] = -1;
//        p[2] = -1;
//        p[3] = NULL;
//        return p;
//    }
//
//    else if(strcmp(proto1st,"TAKE") == 0)
//        TAKE(atoi(proto2nd), atoi(proto3rd), proto4th);
//    else if(strcmp(proto1st,"STATUS") == 0)
//        STATUS(atoi(proto2nd), atoi(proto3rd));
//    else if(strcmp(proto1st,"END") == 0) {
//        END(proto2nd);
//        gameon = 0;
//        replyMessage = "END";
//    }
//
//    else
//        printf("Command wasn't recognised\n");



    //printf("\n");
    printf("DECODE: ");
    switch (a->cmd) {
        case HELLO:
            printf("- Cmd: HELLO");
            break;
        case SIZE:
            printf("- Cmd: SIZE");
            break;
        case NACK:
            printf("- Cmd: NACK");
            break;
        case TAKEN:
            printf("- Cmd: TAKEN");
            break;
        case INUSE:
            printf("- Cmd: INUSE");
            break;
        case PLAYERNAME:
            printf("- Cmd: PLAYERNAME");
            break;
        case START:
            printf("- Cmd: START");
            break;
        case END:
            printf("- Cmd: END");
            break;
        case TAKE:
            printf("- Cmd: TAKE");
            break;
        case STATUS:
            printf("- Cmd: STATUS");
            break;
        default:
            printf("- Cmd: ERROR");
            break;
    }
    printf(" - iParam1: %d", a->iParam1);
    printf(" - iParam2: %d", a->iParam2);
    printf(" - sParam1: %s\n", a->sParam1);
}

void encode(struct action* a, char* returnMessage) {
    switch (a->cmd){
        case HELLO:
            strcpy(returnMessage, "HELLO\n");
            break;
        case SIZE:
            sprintf(returnMessage, "SIZE %d\n", a->iParam1);
            break;
        case NACK:
            strcpy(returnMessage, "NACK\n");
            break;
        case TAKEN:
            strcpy(returnMessage, "TAKEN\n");
            break;
        case INUSE:
            strcpy(returnMessage, "INUSE\n");
            break;
        case PLAYERNAME:
            strcpy(returnMessage, a->sParam1);
            break;
        case START:
            strcpy(returnMessage, "START\n");
            break;
        case END:
            sprintf(returnMessage, "END %s\n", a->sParam1);
            break;
        case TAKE:
            sprintf(returnMessage, "TAKE %d %d %s\n", a->iParam1, a->iParam2, a->sParam1);
            break;
        case STATUS:
            sprintf(returnMessage, "STATUS %d %d\n", a->iParam1, a->iParam2);
            break;
        default:
            strcpy(returnMessage, "Error - something went wrong in the encoder");
            break;
    }

    //printf("\n");
    printf("ENCODE: ");
    printf("- String: %s\n", returnMessage);
}
