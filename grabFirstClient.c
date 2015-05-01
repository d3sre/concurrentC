//
// Created by des on 4/13/15.
//

// Source from http://www.linuxhowtos.org/C_C++/socket.htm

#include "grabFirstClient.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "grabProtocolTranslator.h"


/*
 *  Game default parameters, might be exported or serialized
 */

#define TRUE 1
#define FALSE 0
const char *CLIENTNAME = "alice";

int gameon = 0;
int fieldSize = 2;
int n = 0;

int** playfield;



void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void cleanUpClient(int sockfd) {
    gameon = 0;
    close(sockfd);
}

_Bool doSIZE(int playfieldN){
    int x;
    printf("SIZE received");

    // 4 as this is the minimum size
    if (playfieldN < 4){
        return FALSE;
    }

    //create playfield with n-size
    playfield = malloc(playfieldN*sizeof(int*));
    for (x = 0; x < playfieldN; x++){
        playfield[x] = malloc(playfieldN*sizeof(int));
    }


    n = playfieldN;

    return TRUE;
}

_Bool doSTART(struct action* returnAction){
    printf("START received");

    if (returnAction == NULL) {
        return FALSE;
    }

    //simple Strategy
    returnAction->cmd = TAKE;
    returnAction->iParam1 = 0;
    returnAction->iParam2 = 0;
    strcpy(returnAction->sParam1,CLIENTNAME);

    //game on server has started and entered state 2
    gameon = 2;
    return TRUE;
}

_Bool updatePlayfield(struct action* currentAction, struct action* returnAction) {
    //strategy 1
    int nextTriedFieldX;
    int nextTriedFieldY;

    if (currentAction == NULL || returnAction == NULL){
        return FALSE;
    }

    int lastTriedFieldX = currentAction->iParam1;
    int lastTriedFieldY = currentAction->iParam2;

    if (returnAction->cmd == TAKEN){
        playfield[lastTriedFieldX][lastTriedFieldY] = 1;
    }
    else if (returnAction->cmd == INUSE) {
        // other user currently accessing
        playfield[lastTriedFieldX][lastTriedFieldY] = 2;
    }


    if (lastTriedFieldX < n) {
        nextTriedFieldX = lastTriedFieldX+1;
    }
    else {
        nextTriedFieldX = 0;
    }

    if (lastTriedFieldY < n) {
        nextTriedFieldY = lastTriedFieldY+1;
    }
    else {
        nextTriedFieldY = 0;
    }

    returnAction->cmd = TAKE;
    returnAction->iParam1 = nextTriedFieldX;
    returnAction->iParam2 = nextTriedFieldY;
    strcpy(returnAction->sParam1,CLIENTNAME);


    return TRUE;

}

int main(int argc, char *argv[])
{
    int sockfd, portno, length;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    struct action currentAction;
    struct action returnAction;


    char buffer[256];
    char returnMessage[256];
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    // assignes 2nd start parameter as port number
    portno = atoi(argv[2]);
    // SOCK_STREAM for TCP, Domain for Internet, protocol chosen automatically
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    //define servername from start parameter by host database lookup
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    //set bites to zero
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //copy bites from source to destination
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    gameon = 1;
    while(gameon >= 1) {
        printf("Please enter the message: ");
        //clean buffer
        bzero(buffer, 256);
        //gets string from stream stdin, terminated with newline \n
        fgets(buffer, 255, stdin);
        //writes lenght of buffer from buffer to sockfd, length = numbers of written or -1 for error
        length = write(sockfd, buffer, strlen(buffer));
        if (length < 0)
            error("ERROR writing to socket");
        //clean buffer
        bzero(buffer, 256);
        // read 255 bytes from sockfd into buffer
        length = read(sockfd, buffer, 255);
        if (length > 0) {
            // error("ERROR reading from socket");
            //print buffer, terminated by newline \n

            decode(buffer, &currentAction);
            printf("Buffer: %s\n", buffer);

            switch (currentAction.cmd) {
                case SIZE:
                    doSIZE(currentAction.iParam1);
                    break;
                case NACK:
                    cleanUpClient(sockfd);
                    break;
                case START:
                    doSTART(&returnAction);
                    break;
                case INUSE:
                case TAKEN:
                    //updatePlayfield(&currentAction, &returnAction);
                    break;
                case END:
                    printf("Winner was: %s, game has ended\n", currentAction.sParam1);
                    cleanUpClient(sockfd);
                    break;
                default:
                    printf("Assuming player name was received ...: %s", currentAction.sParam1);
                    break;
            }

            encode(&returnAction, returnMessage);

            printf("Gameon: %d", gameon);
        }
    }

    cleanUpClient(sockfd);
    return 0;
}
