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

int sharedMemIDStruct, sharedMemSizeStruct;
key_t sharedMemKeyStruct;

//int n = 0;
int** playfield;

char** player;
int numberPlayers;
int *numberOfSockets;

enum processType {gameplay , sockethandler, childinteraction};

struct sharedVariables{
    int sv_gameLevel;
    int sv_numberOfPlayers;
    int sv_gamePlayfield;
    char** sv_player;
};

//char players[] = { 'one', 'two', 'test'};
int FIELDSIZE = -1;


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void printPlayers(struct sharedVariables *mainSharedVariables, const char *pInfo){
    printf("%s ", pInfo);
    printf("SLL_INFO | SLC_CAT2_GAMEPLAY, Number of players:%d - Playerlist:\n", mainSharedVariables->sv_numberOfPlayers);
    int i;
    for(i=0; i < mainSharedVariables->sv_numberOfPlayers; i++){
        if(mainSharedVariables->sv_player[i] == NULL){
            printf("ID: %d, Name: %s\n", i, "No name yet");
        }
        else {
            printf("ID: %d, Name: %s\n", i, mainSharedVariables->sv_player[i]);
        }
    }

}

void printPlayfield(struct sharedVariables *mainSharedVariables){
    printf("\n");
    printf("SLL_INFO | SLC_CAT2_GAMEPLAY, Playfield:\n");
    int x;
    int y;
    for(y=0; y<(mainSharedVariables->sv_gamePlayfield);y++){
        for(x=0; x< (mainSharedVariables->sv_gamePlayfield); x++) {
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

void cleanUpServer(struct sharedVariables *mainSharedVariables, int newsockfd, int sockfd){
    //### section: cleanup ###
    close(newsockfd);
    close(sockfd);

    if (mainSharedVariables->sv_player != NULL) {
        int i;
        for(i=0; i< mainSharedVariables->sv_numberOfPlayers -1; i++) {
            free(mainSharedVariables->sv_player[i]);
        }
        free(mainSharedVariables->sv_player);
    }
    if (playfield != NULL) {
        int i;
        for(i=0; i< mainSharedVariables->sv_gamePlayfield -1; i++) {
            free(playfield[i]);
        }
        free(playfield);
    }

}

void addPlayer(struct sharedVariables *childSharedVariables, const char* playername) {
    printf("SLL_INFO | SLC_CAT2_GAMEPLAY, ADDPLAYER (new): %s\n", playername);

    if (childSharedVariables->sv_player == NULL) {
        childSharedVariables->sv_numberOfPlayers = 1;
        childSharedVariables->sv_player = malloc(childSharedVariables->sv_numberOfPlayers*sizeof(char*));
        childSharedVariables->sv_player[childSharedVariables->sv_numberOfPlayers - 1] = malloc(256*sizeof(char));
        strcpy(childSharedVariables->sv_player[childSharedVariables->sv_numberOfPlayers - 1], playername);

        //printf("\nADDPLAYER (added): %s\n", player[0]);
    }
    else {
        char** tempPlayer = childSharedVariables->sv_player;

        childSharedVariables->sv_numberOfPlayers++;
        childSharedVariables->sv_player = malloc(childSharedVariables->sv_numberOfPlayers*sizeof(char*));
        int i;
        //copy old players from old array to new one
        for(i = 0; i < childSharedVariables->sv_numberOfPlayers - 1; i++) {
            childSharedVariables->sv_player[i] = malloc(256 * sizeof(char));
            strcpy(childSharedVariables->sv_player[i], tempPlayer[i]);
        }
        childSharedVariables->sv_player[childSharedVariables->sv_numberOfPlayers - 1] = malloc(256*sizeof(char));
        strcpy(childSharedVariables->sv_player[childSharedVariables->sv_numberOfPlayers - 1], playername);

        for(i=0; i< childSharedVariables->sv_numberOfPlayers -1; i++) {
            free(tempPlayer[i]);
        }
        free(tempPlayer);

        //printf("\nADDPLAYER (added): %s\n", player[numberPlayers - 1]);
    }
}

_Bool existPlayer(struct sharedVariables *mainSharedVariables, const char* playername) {
    int i;
    for(i=0; i< mainSharedVariables->sv_numberOfPlayers; i++) {
        if (strcmp(playername,mainSharedVariables->sv_player[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

int getPlayerID(struct sharedVariables *mainSharedVariables, const char* playername){
    if(existPlayer(mainSharedVariables,playername)){
        int i;
        for(i=0; i< mainSharedVariables->sv_numberOfPlayers; i++) {
            if (strcmp(playername,mainSharedVariables->sv_player[i]) == 0) {
                return i;
            }
        }

    }
    return -1;
}


_Bool doHELLO(struct sharedVariables *childSharedVariables, struct action* returnAction) {
    printf("SLL_INFO | SLC_CAT4_SOCKETCOMMUNICATION, HELLO received  with Playfieldsize: %d \n", childSharedVariables->sv_gamePlayfield);

    if (returnAction == NULL)
        return FALSE;


    //TODO check ready parameters
    int successfulSignup = 1;

    if(successfulSignup){
        returnAction->cmd = SIZE;
        returnAction->iParam1 = childSharedVariables->sv_gamePlayfield;
    }
    else {
        returnAction->cmd = NACK;
    }

    return TRUE;
}

_Bool doTAKE(struct sharedVariables *childSharedVariables, int x, int y, char *playerName, struct action* returnAction) {
    printf("SLL_INFO | SLC_CAT4_SOCKETCOMMUNICATION, TAKE received on x:%d y: %d by: %s\n", x, y, playerName);

    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || playerName == NULL || returnAction == NULL)
        return FALSE;

    //TODO program checks if field is in use
    int successfulTake = 1;

    if (!existPlayer(childSharedVariables,playerName))
        addPlayer(childSharedVariables, playerName);

    if (successfulTake) {
        playfield[x][y] = getPlayerID(childSharedVariables,playerName);


        returnAction->cmd = TAKEN;

    }
    else {
        returnAction->cmd = INUSE;
    }

    //printPlayfield();
    //printPlayers();

    return TRUE;

}

_Bool doSTATUS(struct sharedVariables *childSharedVariables, int x, int y, struct action* returnAction) {
    printf("SLL_INFO | SLC_CAT4_SOCKETCOMMUNICATION, Status x:d% y:%d\n",x,y);

    if (x< 0 || x>=childSharedVariables->sv_gamePlayfield || y<0 || y>= childSharedVariables->sv_gamePlayfield || returnAction == NULL)
        return FALSE;

    //TODO check field and return fieldOwner

    int clientid = playfield[x][y];

    returnAction->cmd = PLAYERNAME;
    if (clientid == -1){
        strcpy(returnAction->sParam1,"-");
    }
    else {
        strcpy(returnAction->sParam1, childSharedVariables->sv_player[clientid]);
    }
    return TRUE;
}

_Bool doEND(struct sharedVariables *childSharedVariables, char *playerName, struct action* returnAction) {
    printf("SLL_INFO | SLC_CAT4_SOCKETCOMMUNICATION, END player: %s\n", playerName );

    if (playerName == NULL|| returnAction == NULL)
        return FALSE;

    childSharedVariables->sv_gameLevel = 0;
    gameon = 0;
    returnAction->cmd = END;
    strcpy(returnAction->sParam1, playerName);

    return TRUE;
}
_Bool doSTART(struct sharedVariables *childSharedVariables, struct action* returnAction) {
    //create segment & set permissions
    while(TRUE) {
        if(childSharedVariables->sv_gameLevel != 2){
            //printf("[SLL_INFO | SLC_CAT1_CHILDINTERACTION, Game is not started");
        }
        else {
            //start the game
            printf("SLL_INFO | SLC_CAT2_GAMEPLAY, game level was raised\n");


            gameon = 2;
            printf("SLL_INFO | SLC_CAT4_SOCKETCOMMUNICATION, START all\n");

            returnAction->cmd = START;
            break;
        }
    }

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


    printf("SLL_WARNING | SLC_CAT6_GENERALERRORS, - 1 -\n");
    sharedMemKey = 1234; //like example from website
    sharedMemSize = 1024; //like example from website
    //create segment & set permissions
    if ((sharedMemID = shmget(sharedMemKey, sharedMemSize, IPC_CREAT | 0666)) < 0) {
        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, ERROR when trying to create shared memory\n");
        return 1;
    }
    //attach segment to data space
    if ((numberOfSockets = (int *) shmat(sharedMemID, NULL, 0)) == (int *)-1) {
        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, ERROR when trying to attach segment to data space \n");
        return 1;
    }
    printf("SLL_WARNING | SLC_CAT6_GENERALERRORS, - 2 -\n");
    memset(numberOfSockets,0,sharedMemSize);

    sharedMemKeyStruct = 5678; //like example from website
    sharedMemSizeStruct = 1024; //like example from website
    //create segment & set permissions
    if ((sharedMemIDStruct = shmget(sharedMemKeyStruct, sharedMemSizeStruct, IPC_CREAT | 0666)) < 0) {
        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, ERROR when trying to create shared memory\n");
        return 1;
    }
    //attach segment to data space
    struct sharedVariables *mainSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);
    memset(mainSharedVariables,0,sharedMemSizeStruct);
//    if ((*mainSharedVariables == ) {
//        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, ERROR when trying to attach segment to data space \n");
//        return 1;
//    }
    //memset(numberOfSockets,0,sharedMemSizeStruct);
//    if ((sharedMemory = shmat(sharedMemID, NULL,0)) == (char *) -1){
//        error("ERROR when trying to attach segment to data space \n");
//        return 1;
//    }
    //zero out memory segment
    //memset(sharedMemory,0,sharedMemSize);
//    memset(sharedMemory,0,sharedMemSize);
//    sharedMemS = sharedMemory;

    //strncpy(sharedMemory, numberOfSockets, sharedMemSize);


    printf("SLL_WARNING | SLC_CAT6_GENERALERRORS, - 3 -\n");
    if (argc > 3) {
        fprintf(stderr,"Usage: grabFirstServer <portnumber> <fieldSize>\n");
        exit(1);
    }
    FIELDSIZE = atoi(argv[2]);

    mainSharedVariables->sv_gameLevel=0;
    mainSharedVariables->sv_numberOfPlayers=0;
    mainSharedVariables->sv_gamePlayfield=FIELDSIZE;

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
        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, ERROR opening socket");
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
        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    //raise game level
    gameon=1;
    mainSharedVariables->sv_gameLevel=1;
    pid_t childpid, masterpid;
    _Bool isChild = FALSE;

    masterpid = fork();
    if (masterpid < 0)
        error("SLL_ERROR | SLC_CAT6_GENERALERRORS, error  on master fork");
    if (masterpid == 0){
        //entered first child
        currentProcessType = sockethandler;
    }
    if (masterpid > 0) {
        //still in master PID
        currentProcessType = gameplay;
    }

    //### section: receiving input from client ###

    printf("SLL_WARNING | SLC_CAT6_GENERALERRORS, - 4: Gameon is %d -\n", gameon);
    while (gameon >= 1) {

        //printf("I'm in the loop %d\n", getpid());
        //1. wait for new clients
        //open new socket and return file descriptor
        // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
        if (currentProcessType == sockethandler) {
            printf("[SLL_INFO | SLC_CAT3_SOCKETHANDLER, SockethandlerPID:%d Mode: %d] Entered Sockethandler interaction loop part\n", getpid(), currentProcessType);

            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

            if (newsockfd < 0)
                error("SLL_ERROR | SLC_CAT5_PROCESSDISPATCHING, ERROR on accept");
            else {
                printf("[SLL_INFO | SLC_CAT5_PROCESSDISPATCHING , SockethandlerPID:%d Mode: %d] New connection was accepted (newsockfd): %d\n", getpid(), currentProcessType, newsockfd);
                childpid = fork();

                if (childpid < 0)
                    error("SLL_ERROR | SLC_CAT5_PROCESSDISPATCHING, ERROR on fork");
                if (childpid == 0) {
                    pid_t currentChildPID = getpid();
                    pid_t parentPID = getppid();

                    isChild = TRUE;
                    printf("[SLL_INFO | SLC_CAT5_PROCESSDISPATCHING , ChildinteractionPID:%d Mode : %d] New Child PID is %d with parent %d\n", currentChildPID, currentProcessType, currentChildPID, parentPID);


                    // get segment & set permissions
                    if ((sharedMemID = shmget(sharedMemKey, sharedMemSize, 0666)) < 0) {
                        error("SLL_ERROR | SLC_CAT5_PROCESSDISPATCHING, ERROR when trying to create shared memory\n");
                        return 1;
                    }
                    //attach segment to data space
                    if ((numberOfSockets = (int *)shmat(sharedMemID, NULL, 0)) == (int *)-1) {
                        error("SLL_ERROR | SLC_CAT5_PROCESSDISPATCHING, ERROR when trying to attach segment to data space \n");
                        return 1;
                    }

                    //create segment & set permissions
                    if ((sharedMemIDStruct = shmget(sharedMemKeyStruct, sharedMemSizeStruct, 0666)) < 0) {
                        error("SLL_ERROR | SLC_CAT5_PROCESSDISPATCHING, ERROR when trying to create shared memory\n");
                        return 1;
                    }
                    //attach segment to data space
                    struct sharedVariables *socketSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);

                    socketSharedVariables->sv_numberOfPlayers++;
                    (*numberOfSockets)++;
                    printf("[SLL_INFO | SLC_CAT5_PROCESSDISPATCHING, ChildinteractionPID:%d Mode : %d] New number of sockets: %d\n", currentChildPID, currentProcessType, mainSharedVariables->sv_numberOfPlayers);

                    currentProcessType = childinteraction;
                    // do stuff
                    close(sockfd);
                }
                if (childpid > 0){
                    //still the parent process
                    pid_t currentPID = getpid();
                    isChild = FALSE;
                    printf("[SLL_INFO | SLC_CAT5_PROCESSDISPATCHING, SockethandlerPID:%d Mode: %d] Current PID is %d \n", currentPID, currentProcessType, currentPID);
                }
            }

        }
        if (currentProcessType == childinteraction){
            pid_t currentChildPID = getpid();

            //create segment & set permissions
            if ((sharedMemIDStruct = shmget(sharedMemKeyStruct, sharedMemSizeStruct, 0666)) < 0) {
                error("SLL_ERROR | SLC_CAT1_CHILDINTERACTION, ERROR when trying to create shared memory\n");
                return 1;
            }
            //attach segment to data space
            struct sharedVariables *childSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);

            //printf("[ChildinteractionPID:%d Mode : %d] Entered Child interaction loop part\n", currentChildPID, currentProcessType);

            //printf("I'm in the child %d\n", getpid());

            //wait for sign that game has started to send out start signal


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
                        doHELLO(childSharedVariables, &returnAction);
                        doSTART(childSharedVariables, &returnAction);
                        break;
                    case TAKE:
                        doTAKE(childSharedVariables, currentAction.iParam1, currentAction.iParam2, currentAction.sParam1, &returnAction);
                        break;
                    case STATUS:
                        doSTATUS(childSharedVariables, currentAction.iParam1, currentAction.iParam2, &returnAction);
                        break;
                    case END:
                        doEND(childSharedVariables, currentAction.sParam1, &returnAction);
                        break;
                    default:
                        printf("[SLL_INFO | SLC_CAT1_CHILDINTERACTION, ChildinteractionPID:%d Mode : %d] ERROR on received action\n",currentChildPID, currentProcessType);
                        break;
                }

                //print received message from buffer
                printf("[SLL_INFO | SLC_CAT1_CHILDINTERACTION, ChildinteractionPID:%d Mode : %d] Message received: %s\n", currentChildPID, currentProcessType, buffer);

                //printf("Here is the message: %s\n", buffer);

                // change struct to string
                encode(&returnAction, replyMessage);

                //write response to socket
                length = write(newsockfd, replyMessage, 18);
                if (length < 0) error("SLL_ERROR | SLC_CAT1_CHILDINTERACTION, ERROR writing to socket");
            }
        }
        if (currentProcessType == gameplay){


            pid_t currentPID = getpid();
            printf("[SLL_INFO | SLC_CAT2_GAMEPLAY, GameplayPID:%d Mode: %d] Entered gameplay interaction loop part\n", currentPID, currentProcessType);


            //2. can we start the game yet
            printf("[SLL_INFO | SLC_CAT2_GAMEPLAY, GameplayPID:%d Mode: %d] Number of players:%d\n",currentPID, currentProcessType, mainSharedVariables->sv_numberOfPlayers);
            //printf("[SLL_INFO | SLC_CAT2_GAMEPLAY, GameplayPID:%d Mode: %d] Number of socketsSharedMemory:%d\n", currentPID, currentProcessType, sharedMemory);
            char logstring[25];
            sprintf(logstring, "[GameplayPID:%d Mode: %d]",currentPID, currentProcessType);


            printPlayers(mainSharedVariables, logstring);
            printPlayfield(mainSharedVariables);

            sleep(2);

            if (mainSharedVariables->sv_numberOfPlayers >= (mainSharedVariables->sv_gamePlayfield/2) && mainSharedVariables->sv_gameLevel == 1){
                mainSharedVariables->sv_gameLevel=2;
                printf("[SLL_INFO | SLC_CAT2_GAMEPLAY, Game could be started");

                //doSTART(&returnAction);

            }

            printf("[SLL_INFO | SLC_CAT2_GAMEPLAY, current Shared Memory: GameLevel: %d, Fieldsize: %d, NumberOfPlayers: %d\n",mainSharedVariables->sv_gameLevel, mainSharedVariables->sv_gamePlayfield, mainSharedVariables->sv_numberOfPlayers);



            //3. are we done yet
        }

    }


    cleanUpServer(mainSharedVariables,newsockfd, sockfd);

    shmdt(mainSharedVariables);

    shmctl(sharedMemID, IPC_RMID, NULL);

    return 0;
}

