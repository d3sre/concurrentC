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
#include <sys/shm.h>
#include "grabProtocolTranslator.h"

/*
 *  Game default parameters, might be exported or serialized
 */

#define MAXPLAYER 2 //value for n, start when n/2 reached

#define TRUE 1
#define FALSE 0

int gameon = 0;
char replyMessage[256];

//shared memory definition, source from http://www.lainoox.com/ipc-shared-memory-c/

char c, tmp;
int sharedMemID, sharedMemSize;
key_t sharedMemKey;
char *sharedMemory, *sharedMemS;

//int n = 0;
int** playfield;

char** player;
int numberPlayers;
int *numberOfSockets;

enum processType {gameplay , sockethandler, childinteraction};



//char players[] = { 'one', 'two', 'test'};
int FIELDSIZE = -1;


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void printPlayers(const char *pInfo){
    printf("%s ", pInfo);
    printf("Number of players:%d - Playerlist:\n", numberPlayers);
    int i;
    for(i=0; i < numberPlayers; i++){
        printf("ID: %d, Name: %s\n", i, player[i]);
    }
}

void printPlayfield(){
    printf("\n");
    printf("Playfield:\n");
    int x;
    int y;
    for(y=0; y<FIELDSIZE;y++){
        for(x=0; x< FIELDSIZE; x++) {
            if(playfield[x][y] == -1) {
                printf("-");
            }
            else {
                printf("%d",playfield[x][y]);
            }

        }
        printf("\n");
    }

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
    //printf("\nADDPLAYER (new): %s\n", playername);

    if (player == NULL) {
        numberPlayers = 1;
        player = malloc(numberPlayers*sizeof(char*));
        player[numberPlayers - 1] = malloc(256*sizeof(char));
        strcpy(player[numberPlayers - 1], playername);

        //printf("\nADDPLAYER (added): %s\n", player[0]);
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

        //printf("\nADDPLAYER (added): %s\n", player[numberPlayers - 1]);
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

int getPlayerID(const char* playername){
    if(existPlayer(playername)){
        int i;
        for(i=0; i< numberPlayers; i++) {
            if (strcmp(playername,player[i]) == 0) {
                return i;
            }
        }

    }
    return -1;
}


_Bool doHELLO(struct action* returnAction) {
    printf("HELLO received\n");

    if (returnAction == NULL)
        return FALSE;

    //TODO check ready parameters
    int successfulSignup = 1;

    if(successfulSignup){
        returnAction->cmd = SIZE;
        returnAction->iParam1 = FIELDSIZE;
    }
    else {
        returnAction->cmd = NACK;
    }

    return TRUE;
}

_Bool doTAKE(int x, int y, char *playerName, struct action* returnAction) {
    printf("TAKE received on x:%d y: %d by: %s", x, y, playerName);

    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || playerName == NULL || returnAction == NULL)
        return FALSE;

    //TODO program checks if field is in use
    int successfulTake = 1;

    if (!existPlayer(playerName))
        addPlayer(playerName);

    if (successfulTake) {
        playfield[x][y] = getPlayerID(playerName);


        returnAction->cmd = TAKEN;

    }
    else {
        returnAction->cmd = INUSE;
    }

    //printPlayfield();
    //printPlayers();

    return TRUE;

}

_Bool doSTATUS(int x, int y, struct action* returnAction) {
    printf("Status x:d% y:%d",x,y);

    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || returnAction == NULL)
        return FALSE;

    //TODO check field and return fieldOwner

    int clientid = playfield[x][y];

    returnAction->cmd = PLAYERNAME;
    if (clientid == -1){
        strcpy(returnAction->sParam1,"-");
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
_Bool doSTART(struct action* returnAction) {
    printf("START all");


    gameon = 2;
    returnAction->cmd = START;

    return TRUE;
}


int main(int argc, char *argv[]) {
    //### section: setup server and socket ###
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int length;
    struct action currentAction;
    struct action returnAction;

    enum processType currentProcessType = gameplay;

    pid_t parentProcess = getpid();

    //shared memory definition, source from http://www.lainoox.com/ipc-shared-memory-c/


    sharedMemKey = 1234; //like example from website
    sharedMemSize = 1024; //like example from website
    //create segment & set permissions
    if ((sharedMemID = shmget(sharedMemKey, sharedMemSize, IPC_CREAT | 0666)) < 0) {
        error("ERROR when trying to create shared memory\n");
        return 1;
    }
    //attach segment to data space
    if ((numberOfSockets = (int *) shmat(sharedMemID, NULL, 0)) == (int *)-1) {
        error("ERROR when trying to attach segment to data space \n");
        return 1;
    }
    memset(numberOfSockets,0,sharedMemSize);
//    if ((sharedMemory = shmat(sharedMemID, NULL,0)) == (char *) -1){
//        error("ERROR when trying to attach segment to data space \n");
//        return 1;
//    }
    //zero out memory segment
    //memset(sharedMemory,0,sharedMemSize);
//    memset(sharedMemory,0,sharedMemSize);
//    sharedMemS = sharedMemory;

    //strncpy(sharedMemory, numberOfSockets, sharedMemSize);



    if (argc > 3) {
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
    int y;
    for(x= 0; x < FIELDSIZE; x++){
        for(y = 0; y < FIELDSIZE; y++){
            playfield[x][y] = -1;
        }
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
    pid_t childpid, masterpid;
    _Bool isChild = FALSE;

    masterpid = fork();
    if (masterpid < 0)
        error("error  on master fork");
    if (masterpid == 0){
        //entered first child
        currentProcessType = sockethandler;
    }
    if (masterpid > 0) {
        //still in master PID
        currentProcessType = gameplay;
    }

    //### section: receiving input from client ###


    while (gameon >= 1) {

        //printf("I'm in the loop %d\n", getpid());
        //1. wait for new clients
        //open new socket and return file descriptor
        // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
        if (currentProcessType == sockethandler) {
            printf("[SockethandlerPID:%d Mode: %d] Entered Sockethandler interaction loop part\n", getpid(), currentProcessType);

            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

            if (newsockfd < 0)
                error("ERROR on accept");
            else {
                printf("[SockethandlerPID:%d Mode: %d] New connection was accepted (newsockfd): %d\n", getpid(), currentProcessType, newsockfd);
                childpid = fork();

                if (childpid < 0)
                    error("ERROR on fork");
                if (childpid == 0) {
                    pid_t currentChildPID = getpid();
                    pid_t parentPID = getppid();

                    isChild = TRUE;
                    printf("[ChildinteractionPID:%d Mode : %d] New Child PID is %d with parent %d\n", currentChildPID, currentProcessType, currentChildPID, parentPID);


                    // get segment & set permissions
                    if ((sharedMemID = shmget(sharedMemKey, sharedMemSize, 0666)) < 0) {
                        error("ERROR when trying to create shared memory\n");
                        return 1;
                    }
                    //attach segment to data space
                    if ((numberOfSockets = (int *)shmat(sharedMemID, NULL, 0)) == (int *)-1) {
                        error("ERROR when trying to attach segment to data space \n");
                        return 1;
                    }

                    (*numberOfSockets)++;
                    printf("[ChildinteractionPID:%d Mode : %d] New number of sockets: %d\n", currentChildPID, currentProcessType, *numberOfSockets);

                    currentProcessType = childinteraction;
                    // do stuff
                    close(sockfd);
                }
                if (childpid > 0){
                    //still the parent process
                    pid_t currentPID = getpid();
                    isChild = FALSE;
                    printf("[SockethandlerPID:%d Mode: %d] Current PID is %d \n", currentPID, currentProcessType, currentPID);
                }
            }

        }
        if (currentProcessType == childinteraction){
            pid_t currentChildPID = getpid();
            //printf("[ChildinteractionPID:%d Mode : %d] Entered Child interaction loop part\n", currentChildPID, currentProcessType);

            //printf("I'm in the child %d\n", getpid());


            //do read game input commands if a child

            // clear buffer
            bzero(buffer, 256);
            //read newsockfd to buffer
            length = read(newsockfd, buffer, 255);
            if (length > 0) {
                //error("ERROR reading from socket");

                // change buffer (string) to struct action
                decode(buffer, &currentAction);

                //possible receiving commands
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
                        printf("[ChildinteractionPID:%d Mode : %d] ERROR on received action\n",currentChildPID, currentProcessType);
                        break;
                }

                //print received message from buffer
                printf("[ChildinteractionPID:%d Mode : %d] Message received: %s\n", currentChildPID, currentProcessType, buffer);

                //printf("Here is the message: %s\n", buffer);

                // change struct to string
                encode(&returnAction, replyMessage);

                //write response to socket
                length = write(newsockfd, replyMessage, 18);
                if (length < 0) error("ERROR writing to socket");
            }
        }
        if (currentProcessType == gameplay){
            pid_t currentPID = getpid();
            printf("[GameplayPID:%d Mode: %d] Entered gameplay interaction loop part\n", currentPID, currentProcessType);


            //2. can we start the game yet
            printf("[GameplayPID:%d Mode: %d] Number of sockets:%d\n",currentPID, currentProcessType, *numberOfSockets);
            printf("[GameplayPID:%d Mode: %d] Number of socketsSharedMemory:%d\n", currentPID, currentProcessType, sharedMemory);
            char logstring[25];
            sprintf(logstring, "[GameplayPID:%d Mode: %d]",currentPID, currentProcessType);


            printPlayers(logstring);
            printPlayfield();

            sleep(10);

            if (*numberOfSockets >= (FIELDSIZE/2) && gameon == 1){
                doSTART(&returnAction);

            }


            //3. are we done yet
        }

    }


    cleanUpServer(newsockfd, sockfd);

    shmdt(numberOfSockets);
    shmctl(sharedMemID, IPC_RMID, NULL);

    return 0;
}

