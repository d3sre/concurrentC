//
// Created by des on 4/13/15.
//

#include "grabFirstServer.h"
/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
// Source from http://www.linuxhowtos.org/C_C++/socket.htm
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/shm.h>
#include "grabProtocolTranslator.h"
#include "SimpleLogger.h"


/*
 *  Game default parameters, might be exported or serialized
 */

#define MAXPLAYER 2 //value for n, start when n/2 reached

#define TRUE 1
#define FALSE 0

int gameon = 0;
char replyMessage[256];

// Global defaults
#define MAX_CLIENTS 256
#define MAX_PLAYER_NAME_LENGTH 256 // Including ending '\0'

// Shared memory mutex names
const char* SHMMN_STATUS_VARIABLES = "/STATUS-VARIABLES-MUTEX";
const char* SHMMN_PLAYER_LIST = "/PLAYER-LIST-MUTEX";
const char* SHMMN_PLAYFIELD_ELEMENT_PREFIX = "/STATUS-VARAIBLES-MUTEX-";

// Shared memory keys
#define SHMK_STATUS_VARIABLES 1000
#define SHMK_PLAYER_LIST 2000
#define SHMK_PLAYFIELD_ROOT 3000

// Shared memory mutexes
sem_t* mtxStatusVariables;
sem_t *mtxPlayerList;
sem_t** mtxPlayfieldElement;

// Shared memory for player list (assumed: max. 256 clients, max. name 255 chars)
int shmPlayerListID;
int shmPlayerListMemSize;
char* shmPlayerList;

int sharedMemIDStruct, sharedMemSizeStruct;

//int n = 0;
int** playfield;

char** player;
int numberPlayers;

enum processType {gameplay , sockethandler, childinteraction};

struct sharedVariables{
    int sv_gameLevel;
    int sv_numberOfPlayers;
    int sv_numberOfPlayerNames;
    int sv_gamePlayfield;
};

//char players[] = { 'one', 'two', 'test'};
int FIELDSIZE = -1;


void error(int mask_flag, const char *msg)
{
    log_printf(mask_flag,msg);
    exit(1);
}

void printPlayers(struct sharedVariables *mainSharedVariables, const char *pInfo){
    /*//TODO printf("%s ", pInfo);
    printf("SLL_INFO | SLC_CAT2_GAMEPLAY, Number of players:%d - Playerlist:\n", mainSharedVariables->sv_numberOfPlayers);
    int i;
    for(i=0; i < mainSharedVariables->sv_numberOfPlayers; i++){
        if(mainSharedVariables->sv_player[i] == NULL){
            printf("ID: %d, Name: %s\n", i, "No name yet");
        }
        else {
            printf("ID: %d, Name: %s\n", i, mainSharedVariables->sv_player[i]);
        }
    }*/

    //TODO printf("%s ", pInfo);

    int i;

    sem_wait(mtxPlayerList);
    sem_wait(mtxStatusVariables);

    log_printf(SLL_INFO|SLC_GAMEPLAY, "PlayerList: Number of players: %d\n", mainSharedVariables->sv_numberOfPlayers);
    log_printf(SLL_INFO|SLC_GAMEPLAY, "PlayerList: Number of player names: %d\n", mainSharedVariables->sv_numberOfPlayerNames);
    log_printf(SLL_INFO|SLC_GAMEPLAY, "PlayerList:\n");

    for (i = 0; i < mainSharedVariables->sv_numberOfPlayerNames; i++) {
        log_printf(SLL_INFO|SLC_GAMEPLAY, "- ID: %d, Name: %s\n", i, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]);
    }

    sem_post(mtxStatusVariables);
    sem_post(mtxPlayerList);
}

void printPlayfield(struct sharedVariables *mainSharedVariables){

    log_printf(SLL_INFO|SLC_GAMEPLAY, "Playfield:\n");
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
    log_printf(SLL_INFO|SLC_GAMEPLAY, "in cleanUpServer function\n");

    //TODO Mutex udn Shared Memories aufräumen


    if (playfield != NULL) {
        int i;
        for(i=0; i< mainSharedVariables->sv_gamePlayfield -1; i++) {
            free(playfield[i]);
        }
        free(playfield);
    }

    shutdown_logger();
}

void addPlayer(struct sharedVariables *childSharedVariables, const char* playername) {
    log_printf(SLL_INFO|SLC_GAMEPLAY, "in addPlayer function\n");
    /*printf("SLL_INFO | SLC_CAT2_GAMEPLAY, ADDPLAYER (new): %s\n", playername);


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
    }*/

    // Wait for necessary locks
    sem_wait(mtxPlayerList);
    sem_wait(mtxStatusVariables);

    strcpy(&shmPlayerList[childSharedVariables->sv_numberOfPlayerNames*MAX_PLAYER_NAME_LENGTH], playername);
    childSharedVariables->sv_numberOfPlayerNames++;

    sem_post(mtxStatusVariables);
    sem_post(mtxPlayerList);
}

_Bool existPlayer(struct sharedVariables *mainSharedVariables, const char* playername) {
    log_printf(SLL_INFO|SLC_GAMEPLAY, "in existPlayer function\n");
    /*int i;
    for(i=0; i< mainSharedVariables->sv_numberOfPlayers; i++) {
        if (strcmp(playername,mainSharedVariables->sv_player[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;*/

    int i;
    _Bool result = FALSE;

    // Wait for necessary locks
    sem_wait(mtxPlayerList);
    sem_wait(mtxStatusVariables);

    for (i = 0; i < mainSharedVariables->sv_numberOfPlayerNames; i++) {
        if (strcmp(playername, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]) == 0) {
            result = TRUE;
            break;
        }
    }

    sem_post(mtxStatusVariables);
    sem_post(mtxPlayerList);

    return result;
}

int getPlayerID(struct sharedVariables *mainSharedVariables, const char* playername){
    log_printf(SLL_INFO|SLC_GAMEPLAY, "getPlayerID function\n");
    /*if(existPlayer(mainSharedVariables,playername)){
        int i;
        for(i=0; i< mainSharedVariables->sv_numberOfPlayers; i++) {
            if (strcmp(playername,mainSharedVariables->sv_player[i]) == 0) {
                return i;
            }
        }

    }
    return -1;*/

    int i;
    int id = -1;

    if (!existPlayer(mainSharedVariables,playername))
        return id;

    // Wait for necessary locks
    sem_wait(mtxPlayerList);
    sem_wait(mtxStatusVariables);

    for(i = 0; i< mainSharedVariables->sv_numberOfPlayerNames; i++) {
        if (strcmp(playername, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]) == 0) {
            id = i;
            break;
        }
    }

    sem_post(mtxStatusVariables);
    sem_post(mtxPlayerList);

    return id;
}


_Bool doHELLO(struct sharedVariables *childSharedVariables, struct action* returnAction) {
    log_printf(SLL_INFO|SLC_SOCKETCOMMUNICATION, "HELLO received with playfield size: %d\n", childSharedVariables->sv_gamePlayfield);

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
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "TAKE received on x:%d y: %d by: %s\n", x, y, playerName);

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
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "Status x:d% y:%d\n",x,y);

    if (x< 0 || x>=childSharedVariables->sv_gamePlayfield || y<0 || y>= childSharedVariables->sv_gamePlayfield || returnAction == NULL)
        return FALSE;

    //TODO check field and return fieldOwner

    int clientid = playfield[x][y];

    returnAction->cmd = PLAYERNAME;
    if (clientid == -1){
        strcpy(returnAction->sParam1,"-");
    }
    else {
        // Wait for necessary locks
        sem_wait(mtxPlayerList);
        strcpy(returnAction->sParam1, &shmPlayerList[clientid*MAX_PLAYER_NAME_LENGTH]);
        sem_post(mtxPlayerList);
    }
    return TRUE;
}

_Bool doEND(struct sharedVariables *childSharedVariables, char *playerName, struct action* returnAction) {
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "END player: %s\n", playerName);

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
        if (childSharedVariables->sv_gameLevel != 2) {
            log_printf(SLL_IDLE | SLC_CHILDINTERACTION, "Game is not yet started\n");
        }
        else {
            //start the game
            log_printf(SLL_INFO|SLC_GAMEPLAY, "Game level was raised\n");

            gameon = 2;
            log_printf(SLL_INFO|SLC_SOCKETCOMMUNICATION, "START all\n");

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

    if (argc > 3) {
        fprintf(stderr,"Usage: grabFirstServer <portnumber> <fieldSize>\n");
        exit(1);
    }
    FIELDSIZE = atoi(argv[2]);

    // Open shared nemory mutexes
    // Mutex for status variables
    if ((mtxStatusVariables = sem_open(SHMMN_STATUS_VARIABLES, O_CREAT, 0666, 1)) == SEM_FAILED) {
        error(SLL_ERROR | SLC_GENERALERRORS,"semaphore initialization");
        return 1;
    }
    /*DEBUG*/printf("AAA\n");
    // Mutex for player list
    if ((mtxPlayerList = sem_open(SHMMN_PLAYER_LIST, O_CREAT, 0600, 1)) == SEM_FAILED) {
	/*DEBUG*/printf("BBB\n");
        error(SLL_ERROR | SLC_GENERALERRORS,"semaphore initialization");
        return 1;
    }
    // Mutexes for playfield elementst
    int i;
    char shmmnPlayfieldElementName[256];
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sprintf(shmmnPlayfieldElementName, "%s%d", SHMMN_PLAYFIELD_ELEMENT_PREFIX, i);
        if ((mtxStatusVariables = sem_open(shmmnPlayfieldElementName, O_CREAT, 0666, 1)) == SEM_FAILED) {
            error(SLL_ERROR | SLC_GENERALERRORS,"semaphore initialization");
            return 1;
        }
    }

    //shared memory definition, source from http://www.lainoox.com/ipc-shared-memory-c/

    log_printf(SLL_WARNING | SLC_GENERALERRORS, "Step - 1-\n");

    sharedMemSizeStruct = 1024; //like example from website
    //create segment & set permissions
    if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, IPC_CREAT | 0666)) < 0) {
        error(SLL_ERROR | SLC_GENERALERRORS, "ERROR when trying to create shared memory\n");
        return 1;
    }
    //attach segment to data space
    struct sharedVariables *mainSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);
    memset(mainSharedVariables,0,sharedMemSizeStruct);
    log_printf(SLL_WARNING | SLC_GENERALERRORS, "Step - 2-\n");

    // Create Player List in shared memory
    shmPlayerListMemSize = MAX_CLIENTS*MAX_PLAYER_NAME_LENGTH*sizeof(char);
    // Create segment & set permissions
    if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, IPC_CREAT | 0666)) < 0) {
        error(SLL_ERROR | SLC_GENERALERRORS, "ERROR when trying to create shared memory\n");
        return 1;
    }
    // Attach segment to data space
    if ((shmPlayerList = (char*)shmat(shmPlayerListID, NULL, 0)) == (char*)-1) {
        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach segment to data space \n");
        return 1;
    }
    memset(shmPlayerList, 0, shmPlayerListMemSize);

    log_printf(SLL_WARNING | SLC_GENERALERRORS, "Step - 3-\n");

    mainSharedVariables->sv_gameLevel=0;
    mainSharedVariables->sv_numberOfPlayers=0;
    mainSharedVariables->sv_numberOfPlayerNames = 0;
    mainSharedVariables->sv_gamePlayfield=FIELDSIZE;

    
    
    
    /*//DEBUG
    printf("1\n");
    strcpy(&shmPlayerList[0*MAX_PLAYER_NAME_LENGTH], "Patrick Diezi");
    strcpy(&shmPlayerList[1*MAX_PLAYER_NAME_LENGTH], "Désirée Sacher");
    printf("2 %s\n", &shmPlayerList[0*MAX_PLAYER_NAME_LENGTH]);
    printf("3 %s\n", &shmPlayerList[1*MAX_PLAYER_NAME_LENGTH]);
    mainSharedVariables->sv_numberOfPlayerNames = 2;
    printf("4\n");
    //DEBUG*/
    
    
        
    

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
        error(SLL_ERROR | SLC_GENERALERRORS, "ERROR opening socket");
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
        error(SLL_ERROR | SLC_GENERALERRORS, "ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    //raise game level
    gameon=1;
    mainSharedVariables->sv_gameLevel=1;
    pid_t childpid, masterpid;
    _Bool isChild = FALSE;

    masterpid = fork();
    if (masterpid < 0)
        error(SLL_ERROR | SLC_GENERALERRORS, "error on master fork");
    if (masterpid == 0){
        //entered first child
        currentProcessType = sockethandler;

        char socketHandlerLogTag[256];
        sprintf(socketHandlerLogTag, "SOCKETHANDLER:%d", getpid());
        startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLC_SOCKETHANDLER, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);

        // Get access to Player List in shared memory
        // get segment & set permissions
        if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, 0666)) < 0) {
            error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to create shared memory\n");
            return 1;
        }
        //attach segment to data space
        if ((shmPlayerList = (char*)shmat(shmPlayerListID, NULL, 0)) == (char*)-1) {
            error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach segment to data space \n");
            return 1;
        }
    }
    if (masterpid > 0) {
        //still in master PID
        currentProcessType = gameplay;

        char gameplayLogTag[256];
        sprintf(gameplayLogTag, "GAMEPLAY:%d", getpid());
        startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLC_GAMEPLAY, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    }

    //### section: receiving input from client ###

    log_printf(SLL_WARNING | SLC_GENERALERRORS, "Step - 4: Gameon is %d \n", gameon);
    while (gameon >= 1) {

        //printf("I'm in the loop %d\n", getpid());
        //1. wait for new clients
        //open new socket and return file descriptor
        // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
        if (currentProcessType == sockethandler) {
            log_printf(SLL_INFO | SLC_SOCKETHANDLER, "SockethandlerPID:%d Mode: %d] Entered Sockethandler interaction loop part\n", getpid(), currentProcessType);

            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

            if (newsockfd < 0)
                error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR on accept");
            else {
                log_printf(SLL_INFO | SLC_PROCESSDISPATCHING, "SockethandlerPID:%d Mode: %d] New connection was accepted (newsockfd): %d\n", getpid(), currentProcessType, newsockfd);
                childpid = fork();

                if (childpid < 0)
                    error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR on fork");
                if (childpid == 0) {
                    pid_t currentChildPID = getpid();
                    pid_t parentPID = getppid();

                    char childLogTag[256];
                    sprintf(childLogTag, "CHILDINTERACTION:%d", currentChildPID);
                    startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_INFO |SLL_ERROR , SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);

                    // Get access to Player List in shared memory
                    // get segment & set permissions
                    if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, 0666)) < 0) {
                        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to create shared memory\n");
                        return 1;
                    }
                    //attach segment to data space
                    if ((shmPlayerList = (char*)shmat(shmPlayerListID, NULL, 0)) == (char*)-1) {
                        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach segment to data space \n");
                        return 1;
                    }

                    isChild = TRUE;
                    log_printf(SLL_INFO|SLC_PROCESSDISPATCHING, "ChildinteractionPID: %d, Mode: %d, New Child PID is %d with parent %d\n", currentChildPID, currentProcessType, currentChildPID, parentPID);

                    //create segment & set permissions
                    if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, 0666)) < 0) {
                        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to create shared memory\n");
                        return 1;
                    }
                    //attach segment to data space
                    struct sharedVariables *socketSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);

                    socketSharedVariables->sv_numberOfPlayers++;
                    log_printf(SLL_INFO|SLC_PROCESSDISPATCHING, "ChildinteractionPID:%d Mode : %d] New number of sockets: %d\n", currentChildPID, currentProcessType, mainSharedVariables->sv_numberOfPlayers);

                    currentProcessType = childinteraction;

                    // do stuff
                    close(sockfd);
                }
                if (childpid > 0){
                    //still the parent process
                    pid_t currentPID = getpid();
                    isChild = FALSE;
                    //TODO printf("[SLL_INFO | SLC_CAT5_PROCESSDISPATCHING, SockethandlerPID:%d Mode: %d] Current PID is %d \n", currentPID, currentProcessType, currentPID);
                    log_printf(SLL_INFO|SLC_PROCESSDISPATCHING, "SockethandlerPID:%d Mode: %d] Current PID is %d \n", currentPID, currentProcessType, currentPID);
                }
            }

        }
        if (currentProcessType == childinteraction){
            pid_t currentChildPID = getpid();

            //create segment & set permissions
            if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, 0666)) < 0) {
                error(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR when trying to create shared memory\n");
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
                        log_printf(SLL_INFO|SLC_CHILDINTERACTION, "ChildinteractionPID:%d Mode : %d] ERROR on received action\n",currentChildPID, currentProcessType);
                        //printf("[SLL_INFO | SLC_CHILDINTERACTION, ChildinteractionPID:%d Mode : %d] ERROR on received action\n",currentChildPID, currentProcessType);
                        break;
                }

                //print received message from buffer

                log_printf(SLL_INFO|SLC_CHILDINTERACTION, "ChildinteractionPID:%d Mode : %d] Message received: %s\n", currentChildPID, currentProcessType, buffer);

                // change struct to string
                encode(&returnAction, replyMessage);

                //write response to socket
                length = write(newsockfd, replyMessage, 18);
                if (length < 0) error(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR writing to socket");
            }
        }
        if (currentProcessType == gameplay){


            pid_t currentPID = getpid();
            log_printf(SLL_INFO|SLC_GAMEPLAY, "GameplayPID: %d, Mode: %d, Entered gameplay interaction loop part\n", currentPID, currentProcessType);


            //2. can we start the game yet
            log_printf(SLL_INFO|SLC_GAMEPLAY, "GameplayPID:%d Mode: %d] Number of players:%d\n",currentPID, currentProcessType, mainSharedVariables->sv_numberOfPlayers);
            char logstring[25];
            sprintf(logstring, "[GameplayPID:%d Mode: %d]",currentPID, currentProcessType);


            printPlayers(mainSharedVariables, logstring);
            //TODO printPlayfield(mainSharedVariables);

            sleep(2);

            if (mainSharedVariables->sv_numberOfPlayers >= (mainSharedVariables->sv_gamePlayfield/2) && mainSharedVariables->sv_gameLevel == 1){
                mainSharedVariables->sv_gameLevel=2;
                log_printf(SLL_INFO|SLC_GAMEPLAY, "Game could be started");

                //doSTART(&returnAction);

            }

            log_printf(SLL_INFO|SLC_GAMEPLAY, "Current Shared Memory: GameLevel: %d, Fieldsize: %d, NumberOfPlayers: %d, NumberOfPlayerNames: %d\n", mainSharedVariables->sv_gameLevel, mainSharedVariables->sv_gamePlayfield, mainSharedVariables->sv_numberOfPlayers, mainSharedVariables->sv_numberOfPlayerNames);

            //3. are we done yet
        }

    }


    cleanUpServer(mainSharedVariables,newsockfd, sockfd);

    shmdt(mainSharedVariables);

    shmctl(sharedMemIDStruct, IPC_RMID, NULL);

    return 0;
}

