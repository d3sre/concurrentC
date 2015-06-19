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
#include <time.h>
#include "grabProtocolTranslator.h"
#include "SimpleLogger.h"


/*
 *  Game default parameters, might be exported or serialized
 */

#define TRUE 1
#define FALSE 0
char CLIENTNAME[243];

int gameon = 0;
int fieldSize = 2;
int n = 0;

int** playfield;
int clientStrategy;
int lastTriedFieldX = 0;
int lastTriedFieldY = 0;


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
    log_printf(SLL_DEBUG | SLC_SOCKETCOMMUNICATION, "Size received\n");
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "Playfield size: %d\n", playfieldN);

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
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "Start received\n");

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

    int nextTriedFieldX;
    int nextTriedFieldY;
    _Bool moveMade = FALSE;

    if (currentAction == NULL || returnAction == NULL){
        return FALSE;
    }

    //make internal notes of playfield state
    if (currentAction->cmd == TAKEN){
        //mark field as taken
        playfield[lastTriedFieldX][lastTriedFieldY] = 1;
    }
    else if (currentAction->cmd == INUSE) {
        // other user currently accessing
        //mark field as other user
        playfield[lastTriedFieldX][lastTriedFieldY] = 2;
    }
    else if (currentAction->cmd == PLAYERNAME) {
        // other user currently accessing
        //mark field as other user
        if (strcmp(currentAction->sParam1,CLIENTNAME) == 0)
            playfield[lastTriedFieldX][lastTriedFieldY] = 1;
        else if (strcmp(currentAction->sParam1,"-") == 0)
            playfield[lastTriedFieldX][lastTriedFieldY] = 0;
        else
            playfield[lastTriedFieldX][lastTriedFieldY] = 2;
    }

    //strategy 1 implementation
    if(clientStrategy == 1){
        //define next X field
        if (lastTriedFieldX < n-1) {
            nextTriedFieldX = lastTriedFieldX+1;
            moveMade = TRUE;
        }
        else {
            nextTriedFieldX = 0;
        }
        //define next Y field
        if (moveMade == FALSE){
            if (lastTriedFieldY < n-1) {
                nextTriedFieldY = lastTriedFieldY+1;
            }
            else {
                nextTriedFieldY = 0;
            }
        }
        else {
            nextTriedFieldY = lastTriedFieldY;
        }

    }

    if (clientStrategy ==2){
        int counter = lastTriedFieldY*n+lastTriedFieldX;
        log_printf(SLL_DEBUG | SLC_SOCKETCOMMUNICATION, "last y: %d, last x: %d, counter: %d, n: %d\n", lastTriedFieldY, lastTriedFieldX, counter, n);
        counter++;
        if (counter > n*n-1)
            counter = 0;
        nextTriedFieldY = counter/n;
        nextTriedFieldX = counter%n;
        log_printf(SLL_DEBUG | SLC_SOCKETCOMMUNICATION, "next y: %d, next x: %d, counter: %d, n: %d\n", lastTriedFieldY, lastTriedFieldX, counter, n);

    }

    if (clientStrategy == 3){
        nextTriedFieldX = rand()%n;
        nextTriedFieldY = rand()%n;
    }

    if (clientStrategy == 4){
        nextTriedFieldX = rand()%n;
        nextTriedFieldY = rand()%n;
    }

    if (clientStrategy < 4) {
        returnAction->cmd = TAKE;
        returnAction->iParam1 = nextTriedFieldX;
        returnAction->iParam2 = nextTriedFieldY;
        strcpy(returnAction->sParam1,CLIENTNAME);
    }
    else if (clientStrategy == 4) {
        returnAction->cmd = STATUS;
        returnAction->iParam1 = nextTriedFieldX;
        returnAction->iParam2 = nextTriedFieldY;
        strcpy(returnAction->sParam1,"");
    }
    else {
        sleep(1);
    }


    lastTriedFieldX = nextTriedFieldX;
    lastTriedFieldY = nextTriedFieldY;

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
    if ((argc != 3) && (argc != 5)) {
        fprintf(stderr,"usage %s hostname port [clientname strategyID]\n", argv[0]);
        exit(0);
    }

    srand(time(NULL));

    char childLogTag[256];
    sprintf(childLogTag, "CHILD:%d", getpid());
    startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_INFO |SLL_ERROR , SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);

    int strategyInput;

    if(argc==3) {
        //define client player name
        log_printf(SLL_INFO | SLC_GAMEPLAY, "Please define Player Name: \n");
        //clean buffer
        bzero(buffer, 256);
        //gets string from stream stdin, terminated with newline \n
        fgets(buffer, 255, stdin);
        if (strlen(buffer) == 0) {
            strncpy(CLIENTNAME, "alice", 256);
            //*CLIENTNAME = "alice";
        }
        else {
            strncpy(CLIENTNAME, buffer, strlen(buffer));
        }
        log_printf(SLL_INFO | SLC_GAMEPLAY, "Please select the strategy ID: \n");
        log_printf(SLL_INFO | SLC_GAMEPLAY, "1: simple from 0/0 to n/n \n");
        log_printf(SLL_INFO | SLC_GAMEPLAY, "2: simple quick\n");
        log_printf(SLL_INFO | SLC_GAMEPLAY, "3: random\n");
        log_printf(SLL_INFO | SLC_GAMEPLAY, "4: only STATUS check\n");
        log_printf(SLL_INFO | SLC_GAMEPLAY, "5: inactive client\n");
        //clean buffer
        bzero(buffer, 256);
        //gets string from stream stdin, terminated with newline \n
        fgets(buffer, 255, stdin);

        strategyInput = atoi(buffer);

    }
    else {
        strategyInput = atoi(argv[4]);
        strncpy(CLIENTNAME, argv[3], 256);
    }
    switch (strategyInput) {
        case 1:
            clientStrategy = 1;
            break;
        case 2:
            clientStrategy = 2;
            break;
        case 3:
            clientStrategy = 3;
            break;
        case 4:
            clientStrategy = 4;
            break;
        case 5:
            clientStrategy = 5;
            break;
        default:
            clientStrategy = 1;
    }

    // assignes 2nd start parameter as port number
    portno = atoi(argv[2]);
    // SOCK_STREAM for TCP, Domain for Internet, protocol chosen automatically
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        log_printf(SLL_ERROR, "ERROR opening socket\n");
    //define servername from start parameter by host database lookup
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        log_printf(SLL_ERROR, "ERROR, no such host\n");
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
        log_printf(SLL_ERROR, "ERROR connecting\n");
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "%10s: Connected to server\n", CLIENTNAME);


    strncpy(buffer,"HELLO",256);
    //*buffer = "HELLO";

    //writes lenght of buffer from buffer to sockfd, length = numbers of written or -1 for error
    length = write(sockfd, buffer, strlen(buffer));
    if (length < 0)
        log_printf(SLL_ERROR, "ERROR writing to socket\n");
    gameon = 1;
    while(gameon >= 1) {
//        printf("Please enter the message: \n");
//        //clean buffer
//        bzero(buffer, 256);
//        //gets string from stream stdin, terminated with newline \n
//        fgets(buffer, 255, stdin);

        //clean buffer - > waiting for input
        bzero(buffer, 256);
        // read 255 bytes from sockfd into buffer
        length = read(sockfd, buffer, 255);
        if (length > 0) {
            // error("ERROR reading from socket");
            //print buffer, terminated by newline \n

            decode(buffer, &currentAction);
            log_printf(SLL_DEBUG | SLC_SOCKETCOMMUNICATION, "Client Buffer decoded: %s\n", buffer);

            //possible receiving commands
            switch (currentAction.cmd) {
                case SIZE:
                    doSIZE(currentAction.iParam1);
                    continue;
                case NACK:
                    cleanUpClient(sockfd);
                    continue;
                case START:
                    doSTART(&returnAction);
                    break;
                case INUSE:
                case TAKEN:
                    updatePlayfield(&currentAction, &returnAction);

                    if (clientStrategy == 5)
                        continue;

                    break;
                case END:
                    log_printf(SLL_INFO | SLC_GAMEPLAY, "Winner was: %s, game has ended\n", currentAction.sParam1);
                    cleanUpClient(sockfd);
                    continue;
                default:
                    log_printf(SLL_INFO | SLC_GAMEPLAY, "Assuming player name was received ...: %s\n", currentAction.sParam1);;
                    updatePlayfield(&currentAction, &returnAction);
                    break;

            }

            encode(&returnAction, returnMessage);
            //writes lenght of buffer from buffer to sockfd, length = numbers of written or -1 for error
            length = write(sockfd, returnMessage, strlen(returnMessage));
            if (length < 0)
                log_printf(SLL_ERROR, "ERROR writing to socket\n");

            //printf("Gameon: %d\n", gameon);
        }
    }

    cleanUpClient(sockfd);
    return 0;
}
