//
// Created by des on 4/13/15.
//

#include "grabFirstServer.h"
/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
// Source from http://www.linuxhowtos.org/C_C++/socket.htm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "grabProtocolTranslator.h"

/*
 *  Game default parameters, might be exported or serialized
 */

#define MAXPLAYER 2 //value for n, start when n/2 reached

#define TRUE 1
#define FALSE 0

int gameon = 0;
char replyMessage[256];

//int n = 0;
int** playfield;

char** player;
int numberPlayers;


//char players[] = { 'one', 'two', 'test'};
int FIELDSIZE = -1;


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void cleanUpServer(int newsockfd, int sockfd){
    //### section: cleanup ###
    close(newsockfd);
    close(sockfd);

    if (player != NULL) {
        int i;
        for(i=0; i< numberPlayers -1; i++) {
            free(player[i]);
        }
        free(player);
    }
    if (playfield != NULL) {
        int i;
        for(i=0; i< FIELDSIZE -1; i++) {
            free(playfield[i]);
        }
        free(playfield);
    }

}

void addPlayer(const char* playername) {
    if (player == NULL) {
        numberPlayers = 1;
        player = malloc(numberPlayers*sizeof(char*));
        player[numberPlayers - 1] = malloc(256*sizeof(char));
        strcpy(player[numberPlayers - 1], playername);
    }
    else {
        char** tempPlayer = player;

        numberPlayers++;
        player = malloc(numberPlayers*sizeof(char*));
        int i;
        //copy old players from old array to new one
        for(i = 0; i < numberPlayers - 1; i++) {
            player[i] = malloc(256 * sizeof(char));
            strcpy(player[i], tempPlayer[i]);
        }
        player[numberPlayers - 1] = malloc(256*sizeof(char));
        strcpy(player[numberPlayers - 1], playername);

        for(i=0; i< numberPlayers -1; i++) {
            free(tempPlayer[i]);
        }
        free(tempPlayer);
    }


}

_Bool existPlayer(const char* playername) {
    int i;
    for(i=0; i< numberPlayers; i++) {
        if (strcmp(playername,player[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}



_Bool doHELLO(struct action* returnAction) {
    printf("HELLO received");

    if (returnAction == NULL)
        return FALSE;

    //TODO check ready parameters
    int successfulSignup = 1;


    if(successfulSignup){

        returnAction->cmd = SIZE;
        returnAction->iParam2 = FIELDSIZE;
    }
    else {
        returnAction->cmd = NACK;
    }

    return TRUE;
}

_Bool doTAKE(int x, int y, char *playerName, struct action* returnAction) {
    printf("TAKE received on x:%d y: %d by: %s", x, y, playerName);

    if (playerName == NULL || returnAction == NULL)
        return FALSE;

    //TODO program checks if field is in use
    int successfulTake = 1;

    if (existPlayer(playerName))
        addPlayer(playerName);


    if (successfulTake) {
        returnAction->cmd = TAKEN;

    }
    else {
        returnAction->cmd = INUSE;
    }

    return TRUE;

}

_Bool doSTATUS(int x, int y, struct action* returnAction) {
    printf("Status x:d% y:%d",x,y);

    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || returnAction == NULL)
        return FALSE;

    //TODO check field and return fieldOwner

    int clientid = playfield[x][y];

    returnAction->cmd = PLAYERNAME;
    if (clientid == 0){
        strcpy(returnAction->sParam1,"FREE");
    }
    else {
        strcpy(returnAction->sParam1, player[clientid]);
    }
    return TRUE;
}

_Bool doEND(char *playerName, struct action* returnAction) {
    printf("END player: %s", playerName );

    if (playerName == NULL|| returnAction == NULL)
        return FALSE;

    gameon = 0;
    returnAction->cmd = END;
    strcpy(returnAction->sParam1, playerName);

    return TRUE;
}



int main(int argc, char *argv[])
{
    //### section: setup server and socket ###
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int length;
    struct action currentAction;
    struct action returnAction;

    if (argc < 3) {
        fprintf(stderr,"Usage: grabFirstServer <portnumber> <fieldSize>\n");
        exit(1);
    }
    FIELDSIZE = atoi(argv[2]);
    int x;

    //create playfield with n-size
    playfield = malloc(FIELDSIZE*sizeof(int*));
    for (x = 0; x < FIELDSIZE; x++){
        playfield[x] = malloc(FIELDSIZE*sizeof(int));
    }

    // SOCK_STREAM for TCP, Domain for Internet, protocol chosen automatically
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    //set sizeof(serv_addr) of bytes to zero
    bzero((char *) &serv_addr, sizeof(serv_addr));
    //convert cmd input to integer and assign to portno
    portno = atoi(argv[1]);
    //configure serv_addr structure object with server details
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    //bind  socket file descriptor to serv_addr, with length sizeof(serv_addr) and if error print out
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    gameon = 1;
    //### section: receiving input from client ###
    //open new socket and return file descriptor
    // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    while (gameon == 1) {
        // clear buffer
        bzero(buffer, 256);
        //read newsockfd to buffer
        length = read(newsockfd, buffer, 255);
        if (length < 0) error("ERROR reading from socket");

        // change buffer (string) to struct action
        decode(buffer, &currentAction);

        switch (currentAction.cmd) {
            case HELLO:
                doHELLO(&returnAction);
                break;
            case TAKE:
                doTAKE(currentAction.iParam1, currentAction.iParam2, currentAction.sParam1, &returnAction);
                break;
            case STATUS:
                doSTATUS(currentAction.iParam1, currentAction.iParam2, &returnAction);
                break;
            case END:
                doEND(currentAction.sParam1, &returnAction);
                break;
            default:
                error("ERROR on received action");
                break;
        }

        //print received message from buffer
        printf("Here is the message: %s\n", buffer);

        // change struct to string
        encode(&returnAction, replyMessage);

        //write response to socket
        length = write(newsockfd, replyMessage, 18);
        if (length < 0) error("ERROR writing to socket");
    }

    cleanUpServer(newsockfd, sockfd);


    return 0;
}
