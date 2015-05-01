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
        a->iParam1 = -1;
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
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
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
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"END") == 0) {
        a->cmd = END;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else if(strcmp(proto1st,"PLAYERNAME") == 0) {
        a->cmd = PLAYERNAME;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
    }
    else {
        a->cmd = ERROR;
        a->iParam1 = -1;
        a->iParam2 = -1;
        strcpy(a->sParam1, "");
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



    printf("entered parser-2 %s\n", proto1st);
    printf("entered parser-2 %s\n", proto2nd);
    printf("entered parser-2 %s\n", proto3rd);
    printf("entered parser-2 %s\n", proto4th);

}

void encode(struct action* a, const char* returnMessage) {

    char tempReturnMessage[256];
    char *integeParamter1 [4];
    char *integeParamter2 [4];
    char *stringParameter1 [240];

    if (a->cmd == SIZE){
        strcpy(integeParamter1,a->iParam1);
        strcpy(tempReturnMessage, "SIZE ");
        strcat(tempReturnMessage, integeParamter1);
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;

    }
    else if(a->cmd == NACK) {
        strcpy(tempReturnMessage, "NACK");
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }
    else if (a->cmd == TAKEN){
        strcpy(tempReturnMessage, "TAKEN");
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;

    }
    else if (a->cmd == INUSE) {
        strcpy(tempReturnMessage, "INUSE");
        strcat(tempReturnMessage, a->iParam1);
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }

    else if (a->cmd == PLAYERNAME) {
        strcpy(tempReturnMessage, a->sParam1);
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }

    else if (a->cmd == START) {
        strcpy(tempReturnMessage, "START");
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }

    else if (a->cmd == END) {
        strcpy(tempReturnMessage, "END ");
        strcat(tempReturnMessage, a->sParam1);
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }

    else if (a->cmd == TAKE){
        //strtol(a->iParam1,NULL,256);
        strcpy(tempReturnMessage, "TAKE ");
        strcat(tempReturnMessage, a->iParam1);
        strcat(tempReturnMessage, " ");
        strcat(tempReturnMessage, a->iParam2);
        strcat(tempReturnMessage, " ");
        strcat(tempReturnMessage, a->sParam1);
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }

    else if (a->cmd == STATUS){
        strcpy(tempReturnMessage, "STATUS ");
        strcat(tempReturnMessage, a->iParam1);
        strcat(tempReturnMessage, " ");
        strcat(tempReturnMessage, a->iParam2);
        strcat(tempReturnMessage, "\n");
        returnMessage = tempReturnMessage;
    }

    else {
        returnMessage = "Error - something went wrong in the encoder";
    }
}
