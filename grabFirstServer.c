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
#include <signal.h>
#include <sys/wait.h>
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

// Shared memory semaphores names
const char* SHMSN_STATUS_VARIABLES = "/STATUS";
const char* SHMSN_PLAYER_LIST = "/PLAYERLIST";
const char* SHMSN_PLAYFIELD_ELEMENT_PREFIX = "/PLAYFIELDELEMENT";

// Shared memory keys
#define SHMK_STATUS_VARIABLES 1000
#define SHMK_PLAYER_LIST 2000
#define SHMK_PLAYFIELD 3000

// Shared memory semaphores
sem_t* semStatusVariables;
sem_t* semPlayerList;
sem_t** semPlayfieldElement;

// Shared memory for player list (assumed: max. 256 clients, max. name 255 chars)
int shmPlayerListID;
int shmPlayerListMemSize;
char* shmPlayerList;
int* shmPlayfield;
int shmPlayfieldMemSize;
int shmPlayfieldID;

int sharedMemIDStruct, sharedMemSizeStruct;

//int n = 0;
//int** playfield;

enum processType {gameplay , sockethandler, childinteraction};

struct sharedVariables{
    int sv_gameLevel;
    int sv_numberOfPlayers;
    int sv_numberOfPlayerNames;
};

int FIELDSIZE = -1;

static volatile int keepRunning = 1;

void  intHandler(int sig)
{
    printf("\n");
    printf("=== Ctrl-C was hit ... cleaning up ===\n");
    printf("\n");
    keepRunning = 0;
}



void error(int mask_flag, const char *msg)
{
    log_printf(mask_flag,msg);
    exit(1);
}

void printPlayers(struct sharedVariables *mainSharedVariables, const char *pInfo){
    int i;

    log_printf(SLC_DEBUG, "PLAYERLIST B1\n");
    sem_wait(semPlayerList);
    log_printf(SLC_DEBUG, "STATUS B1\n");
    sem_wait(semStatusVariables);


    log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE, "PlayerList: Number of players: %d\n", mainSharedVariables->sv_numberOfPlayers);
    log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE, "PlayerList: Number of player names: %d\n", mainSharedVariables->sv_numberOfPlayerNames);
    log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE, "PlayerList:\n");

    for (i = 0; i < mainSharedVariables->sv_numberOfPlayerNames; i++) {
        log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE, "- ID: %d, Name: %s\n", i, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]);
    }

    sem_post(semStatusVariables);
    log_printf(SLC_DEBUG, "STATUS A1\n");
    sem_post(semPlayerList);
    log_printf(SLC_DEBUG, "PLAYERLIST B1\n");
}

void printPlayfield(){
    log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE, "Playfield:\n");
    int x;
    int y;
    log_printf(SLC_DEBUG, "STATUS B2\n");
    // Lock all semaphores for playfield elementst
    int i;
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        log_printf(SLC_DEBUG, "PLAYFIELD SEM WAIT %d\n", i);
        sem_wait(semPlayfieldElement[i]);
    }

    int index;

    for(y=0; y<FIELDSIZE;y++){
        log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE,"");
        for(x=0; x< FIELDSIZE; x++) {
            index = FIELDSIZE*y+x;

            if(shmPlayfield[index] == -1) {
                log_appendprintf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE,"-");
            }
            else {
                log_appendprintf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE,"%d",shmPlayfield[index]);
            }

        }
        log_appendprintf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE,"\n");
    }
    //unlock all playfield element semaphores
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        log_printf(SLC_DEBUG, "PLAYFIELD SEM POST %d\n", i);
        sem_post(semPlayfieldElement[i]);
    }
    log_printf(SLC_DEBUG, "STATUS A2\n");

}

void cleanUpGameplay(struct sharedVariables *mainSharedVariables, int socket) {
    int i;
    char shmmnPlayfieldElementName[256];

    log_printf(SLL_INFO|SLC_GAMEPLAY, "In cleanUpGameplay function\n");

    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 1\n");
    // Close socket
    close(socket);
    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 2\n");

    // Close semaphores
    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 3\n");
    sem_close(semStatusVariables);
    sem_unlink(SHMSN_STATUS_VARIABLES);
    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 4\n");
    sem_close(semPlayerList);
    sem_unlink(SHMSN_PLAYER_LIST);

    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sem_close(semPlayfieldElement[i]);
        sprintf(shmmnPlayfieldElementName, "%s%d", SHMSN_PLAYFIELD_ELEMENT_PREFIX, i);
        sem_unlink(shmmnPlayfieldElementName);
    }
    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 5\n");
    free(semPlayfieldElement);

    // Cleanup shared memeory
    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 6\n");
    shmdt(shmPlayerList);
    shmPlayerList = NULL;
    shmctl(shmPlayerListID, IPC_RMID, NULL);

    // Cleanup playfield in shared memory
    shmdt(shmPlayfield);
    shmPlayfield = NULL;
    shmctl(shmPlayfieldID, IPC_RMID, NULL);

    log_printf(SLC_DEBUG, "CLEANUP GAMEPLAY 7\n");
    shmdt(mainSharedVariables);
    mainSharedVariables = NULL;
    shmctl(sharedMemIDStruct, IPC_RMID, NULL);

    // Shutdown logger
    shutdown_logger();
}

void cleanUpSocketHandler(int socket) {
    log_printf(SLL_INFO|SLC_GAMEPLAY, "In cleanUpSocketHandler function\n");

    // Close socket
    close(socket);

    // Cleanup shared memeory
    shmdt(shmPlayerList);
    shmPlayerList = NULL;
    shmctl(shmPlayerListID, IPC_RMID, NULL);

    // Shutdown logger
    shutdown_logger();
}

void cleanUpChildInteraction(struct sharedVariables *mainSharedVariables) {
    log_printf(SLL_INFO|SLC_GAMEPLAY, "In cleanUpChildInteraction function\n");

    // Cleanup shared memeory
    shmdt(shmPlayerList);
    shmPlayerList = NULL;
    shmctl(shmPlayerListID, IPC_RMID, NULL);

    shmdt(mainSharedVariables);
    mainSharedVariables = NULL;
    shmctl(sharedMemIDStruct, IPC_RMID, NULL);

    shmdt(shmPlayfield);
    shmPlayfield = NULL;
    shmctl(shmPlayfieldID, IPC_RMID, NULL);

    // Shutdown logger
    shutdown_logger();
}

void addPlayer(struct sharedVariables *mainSharedVariables, const char* playername) {
    log_printf(SLL_INFO|SLC_GAMEPLAY, "in addPlayer function\n");

    // Wait for necessary locks
    log_printf(SLC_DEBUG, "PLAYERLIST B2\n");
    sem_wait(semPlayerList);
    log_printf(SLC_DEBUG, "STATUS B4\n");
    sem_wait(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");

    strcpy(&shmPlayerList[mainSharedVariables->sv_numberOfPlayerNames*MAX_PLAYER_NAME_LENGTH], playername);
    mainSharedVariables->sv_numberOfPlayerNames++;

    sem_post(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
    log_printf(SLC_DEBUG, "STATUS A4\n");
    sem_post(semPlayerList);
    log_printf(SLC_DEBUG, "PLAYERLIST A2\n");
}

_Bool existPlayer(struct sharedVariables *mainSharedVariables, const char* playername) {
    log_printf(SLL_INFO|SLC_GAMEPLAY, "in existPlayer function\n");

    int i;
    _Bool result = FALSE;

    // Wait for necessary locks
    log_printf(SLC_DEBUG, "PLAYERLIST B3\n");
    sem_wait(semPlayerList);
    log_printf(SLC_DEBUG, "STATUS B5\n");
    sem_wait(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Number Players: %d\n", mainSharedVariables->sv_numberOfPlayerNames);

    for (i = 0; i < mainSharedVariables->sv_numberOfPlayerNames; i++) {
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "in FOR loop at ID: %d\n", i);
        if (strcmp(playername, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]) == 0) {
            result = TRUE;
            log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Player found at ID %d\n", i);
            break;
        }
    }

    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "After FOR loop\n");

    sem_post(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
    log_printf(SLC_DEBUG, "STATUS A5\n");
    sem_post(semPlayerList);
    log_printf(SLC_DEBUG, "PLAYERLIST A3\n");

    return result;
}

int getPlayerID(struct sharedVariables *mainSharedVariables, const char* playername){
    log_printf(SLL_INFO|SLC_GAMEPLAY, "getPlayerID function\n");

    int i;
    int id = -1;

    if (!existPlayer(mainSharedVariables,playername))
        return id;

    // Wait for necessary locks
    log_printf(SLC_DEBUG, "PLAYERLIST B4\n");
    sem_wait(semPlayerList);
    log_printf(SLC_DEBUG, "STATUS B6\n");
    sem_wait(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");

    for(i = 0; i< mainSharedVariables->sv_numberOfPlayerNames; i++) {
        if (strcmp(playername, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]) == 0) {
            id = i;
            break;
        }
    }

    sem_post(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
    log_printf(SLC_DEBUG, "STATUS A6\n");
    sem_post(semPlayerList);
    log_printf(SLC_DEBUG, "PLAYERLIST A4\n");

    return id;
}


_Bool doHELLO(struct action* returnAction) {
    log_printf(SLC_DEBUG, "STATUS B7\n");
    log_printf(SLL_INFO|SLC_SOCKETCOMMUNICATION, "HELLO received with playfield size: %d\n", FIELDSIZE);
    log_printf(SLC_DEBUG, "STATUS A7\n");

    if (returnAction == NULL)
        return FALSE;

    //TODO check ready parameters
    int successfulSignup = 1;
 
    if(successfulSignup){
        returnAction->cmd = SIZE;
        log_printf(SLC_DEBUG, "STATUS B8\n");
        returnAction->iParam1 = FIELDSIZE;
        log_printf(SLC_DEBUG, "STATUS A8\n");
    }
    else {
	returnAction->cmd = NACK;
    }

    return TRUE;
}

_Bool doTAKE(struct sharedVariables *mainSharedVariables, int x, int y, char *playerName, struct action* returnAction) {
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "TAKE received on x:%d y: %d by: %s\n", x, y, playerName);

    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || playerName == NULL || returnAction == NULL)
        return FALSE;

    int index = y*FIELDSIZE+x;

    log_printf(SLC_DEBUG_SEM_STATUS, "Before existPlayer\n");
    if (!existPlayer(mainSharedVariables,playerName)) {
        log_printf(SLC_DEBUG_SEM_STATUS, "Before addPlayer\n");
        addPlayer(mainSharedVariables, playerName);
    }

    if (sem_trywait(semPlayfieldElement[index])== 0) {
        shmPlayfield[index] = getPlayerID(mainSharedVariables,playerName);
        sem_post(semPlayfieldElement[index]);

        returnAction->cmd = TAKEN;

    }
    else {
        returnAction->cmd = INUSE;
    }

    return TRUE;

}

_Bool doSTATUS(int x, int y, struct action* returnAction) {
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION, "Status x:d% y:%d\n",x,y);
    log_printf(SLC_DEBUG, "STATUS B9\n");
    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || returnAction == NULL){
        log_printf(SLC_DEBUG, "STATUS A9A\n");
        return FALSE;
    }
    log_printf(SLC_DEBUG, "STATUS A9B\n");

    int index = y*FIELDSIZE+x;

    sem_wait(semPlayfieldElement[index]);
    int clientid = shmPlayfield[index];
    sem_post(semPlayfieldElement[index]);

    returnAction->cmd = PLAYERNAME;
    if (clientid == -1){
        strcpy(returnAction->sParam1,"-");
    }
    else {
        // Wait for necessary locks
        log_printf(SLC_DEBUG, "PLAYERLIST B5\n");
        sem_wait(semPlayerList);
        strcpy(returnAction->sParam1, &shmPlayerList[clientid*MAX_PLAYER_NAME_LENGTH]);
        sem_post(semPlayerList);
        log_printf(SLC_DEBUG, "PLAYERLIST A5\n");
    }
    return TRUE;
}

_Bool doEND(struct sharedVariables *mainSharedVariables, char *playerName, struct action* returnAction) {
    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION|SLC_RELEASE, "END winner: %s\n", playerName);

    if (playerName == NULL|| returnAction == NULL)
        return FALSE;
    log_printf(SLC_DEBUG, "STATUS B10\n");
    sem_wait(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
    mainSharedVariables->sv_gameLevel = 0;
    sem_post(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
    log_printf(SLC_DEBUG, "STATUS A10\n");
    gameon = 0;
    returnAction->cmd = END;
    strcpy(returnAction->sParam1, playerName);

    return TRUE;
}

_Bool doSTART(struct sharedVariables *mainSharedVariables, struct action* returnAction) {
    //create segment & set permissions
    while(TRUE) {
        log_printf(SLC_DEBUG, "STATUS B11\n");
        sem_wait(semStatusVariables);
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
        if (mainSharedVariables->sv_gameLevel != 2) {
            log_printf(SLL_IDLE | SLC_CHILDINTERACTION, "Game is not yet started\n");
        }
        else {
            //start the game
            log_printf(SLL_INFO|SLC_GAMEPLAY, "Game level was raised\n");

            gameon = 2;
            log_printf(SLL_INFO|SLC_SOCKETCOMMUNICATION|SLC_RELEASE, "START all\n");

            returnAction->cmd = START;
            sem_post(semStatusVariables);
            log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
            log_printf(SLC_DEBUG, "STATUS A11A\n");
            break;
        }
        sem_post(semStatusVariables);
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
        log_printf(SLC_DEBUG, "STATUS A11B\n");
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
    char loglevel[10];

    char gameplayLogTag[256];

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = intHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    //convert input variables
    portno = atoi(argv[1]);
    FIELDSIZE = atoi(argv[2]);
    if(argc == 4){
        if((strcmp(argv[3],"RELEASE")==0) ||(strcmp(argv[3],"DEBUG")==0)){
            strncpy(loglevel, argv[3], 10);
        }
        else {
            strncpy(loglevel,"RELEASE",10);
        }
    }
    else {
        strncpy(loglevel,"RELEASE",10);
    }


    sprintf(gameplayLogTag, "GAMEPLAY:%d", getpid());
    //startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLC_GAMEPLAY, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    if(strcmp(loglevel,"RELEASE") == 0) {
        startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLL_FATAL| SLL_ERROR |SLC_RELEASE, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    }
    else {
        startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLC_GAMEPLAY, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    }

    
    enum processType currentProcessType = gameplay;

    pid_t parentProcess = getpid();

    if ((argc != 3) && (argc != 4)) {
        fprintf(stderr,"Usage: grabFirstServer <portnumber> <fieldSize> [RELEASE | DEBUG]\n");
        exit(1);
    }


    // Open shared nemory semaphores
    // semaphore for status variables
    if ((semStatusVariables = sem_open(SHMSN_STATUS_VARIABLES, O_CREAT, 0600, 1)) == SEM_FAILED) {
        error(SLL_ERROR | SLC_GENERALERRORS,"semaphore initialization");
        return 1;
    }

    // semaphore for player list
    if ((semPlayerList = sem_open(SHMSN_PLAYER_LIST, O_CREAT, 0600, 1)) == SEM_FAILED) {

        error(SLL_ERROR | SLC_GENERALERRORS,"semaphore initialization");
        return 1;
    }



    // semaphores for playfield elementst
    int i;
    char shmmnPlayfieldElementName[256];
    semPlayfieldElement = malloc(FIELDSIZE*FIELDSIZE*sizeof(sem_t));
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sprintf(shmmnPlayfieldElementName, "%s%d", SHMSN_PLAYFIELD_ELEMENT_PREFIX, i);
        if ((semPlayfieldElement[i] = sem_open(shmmnPlayfieldElementName, O_CREAT, 0600, 1)) == SEM_FAILED) {
            error(SLL_ERROR | SLC_GENERALERRORS,"semaphore initialization");
            return 1;
        }
    }

    //shared memory definition, source from http://www.lainoox.com/ipc-shared-memory-c/

    log_printf(SLL_WARNING | SLC_GENERALERRORS, "Step - 1-\n");

    sharedMemSizeStruct = sizeof(struct sharedVariables); //like example from website
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

    // Create Playerfield in shared memory
    shmPlayfieldMemSize = FIELDSIZE*FIELDSIZE*sizeof(int);
    // Create segment & set permissions
    if ((shmPlayfieldID = shmget(SHMK_PLAYFIELD, shmPlayfieldMemSize, IPC_CREAT | 0666)) < 0) {
        error(SLL_ERROR | SLC_GENERALERRORS, "ERROR when trying to create shared memory for playfield\n");
        return 1;
    }
    // Attach segment to data space
    if ((shmPlayfield = (int*)shmat(shmPlayfieldID, NULL, 0)) == (int*)-1) {
        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach segment to data space for playfield\n");
        return 1;
    }
    memset(shmPlayfield, -1, shmPlayfieldMemSize);


    // SOCK_STREAM for TCP, Domain for Internet, protocol chosen automatically
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error(SLL_ERROR | SLC_GENERALERRORS, "ERROR opening socket");
    //set sizeof(serv_addr) of bytes to zero
    bzero((char *) &serv_addr, sizeof(serv_addr));
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
        //startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLC_SOCKETHANDLER, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
        if(strcmp(loglevel,"RELEASE") == 0) {
            startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLL_FATAL| SLL_ERROR |SLC_RELEASE, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
        }
        else {
            startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLC_SOCKETHANDLER, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
        }

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
    }

    //### section: receiving input from client ###

    log_printf(SLL_WARNING | SLC_GENERALERRORS, "Step - 4: Gameon is %d \n", gameon);

    log_printf(SLC_DEBUG, "STATUS B14A\n");
    sem_wait(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
    while ((mainSharedVariables->sv_gameLevel >= 1) && (keepRunning == 1)) {
        sem_post(semStatusVariables);
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
        log_printf(SLC_DEBUG, "STATUS A14A\n");

        //printf("I'm in the loop %d\n", getpid());
        //1. wait for new clients
        //open new socket and return file descriptor
        // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
        if (currentProcessType == sockethandler) {
            log_printf(SLL_INFO | SLC_SOCKETHANDLER, "SockethandlerPID:%d Mode: %d] Entered Sockethandler interaction loop part\n", getpid(), currentProcessType);

            log_printf(SLC_DEBUG, "COREDUMP 1\n");
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            log_printf(SLC_DEBUG, "COREDUMP 2\n");

            if (newsockfd < 0) {
                log_printf(SLC_DEBUG, "COREDUMP 3\n");
                error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR on accept");
                log_printf(SLC_DEBUG, "COREDUMP 4\n");
            }
            else {
                log_printf(SLC_DEBUG, "COREDUMP 5\n");
                log_printf(SLL_INFO | SLC_PROCESSDISPATCHING|SLC_RELEASE, "SockethandlerPID:%d Mode: %d] New connection was accepted (newsockfd): %d\n", getpid(), currentProcessType, newsockfd);
                log_printf(SLC_DEBUG, "COREDUMP 6\n");
                childpid = fork();

                if (childpid < 0)
                    error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR on fork");
                if (childpid == 0) {
                    pid_t currentChildPID = getpid();
                    pid_t parentPID = getppid();

                    char childLogTag[256];
                    sprintf(childLogTag, "CHILDINTERACTION:%d", currentChildPID);
                    //startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_INFO |SLL_ERROR , SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
                    if(strcmp(loglevel,"RELEASE") == 0) {
                        startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_FATAL| SLL_ERROR |SLC_RELEASE, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
                    }
                    else {
                        startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_INFO |SLL_ERROR, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
                    }

                    // Get access to Player List in shared memory
                    // get segment & set permissions
                    if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, 0666)) < 0) {
                        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to create shared memory\n");
                        log_printf(SLC_DEBUG, "STATUS B15\n");
                        sem_wait(semStatusVariables);
                        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
                        mainSharedVariables->sv_gameLevel = 0;
                        sem_post(semStatusVariables);
                        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                        log_printf(SLC_DEBUG, "STATUS A15\n");
                        break;
                    }
                    //attach segment to data space
                    if ((shmPlayerList = (char*)shmat(shmPlayerListID, NULL, 0)) == (char*)-1) {
                        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach segment to data space \n");
                        log_printf(SLC_DEBUG, "STATUS B16\n");
                        sem_wait(semStatusVariables);
                        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
                        mainSharedVariables->sv_gameLevel = 0;
                        sem_post(semStatusVariables);
                        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                        log_printf(SLC_DEBUG, "STATUS A16\n");
                        break;
                    }

                    isChild = TRUE;
                    log_printf(SLL_INFO|SLC_PROCESSDISPATCHING, "ChildinteractionPID: %d, Mode: %d, New Child PID is %d with parent %d\n", currentChildPID, currentProcessType, currentChildPID, parentPID);

                    //create segment & set permissions
                    if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, 0666)) < 0) {
                        error(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to create shared memory\n");
                        log_printf(SLC_DEBUG, "STATUS B17\n");
                        sem_wait(semStatusVariables);
                        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
                        mainSharedVariables->sv_gameLevel = 0;
                        sem_post(semStatusVariables);
                        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                        log_printf(SLC_DEBUG, "STATUS A17\n");
                        break;
                    }
                    //attach segment to data space
                    mainSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);
                    log_printf(SLC_DEBUG, "STATUS B12\n");
                    sem_wait(semStatusVariables);
                    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
                    mainSharedVariables->sv_numberOfPlayers++;
                    log_printf(SLL_INFO|SLC_PROCESSDISPATCHING, "ChildinteractionPID:%d Mode : %d] New number of sockets: %d\n", currentChildPID, currentProcessType, mainSharedVariables->sv_numberOfPlayers);
                    sem_post(semStatusVariables);
                    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                    log_printf(SLC_DEBUG, "STATUS A12\n");
                    currentProcessType = childinteraction;

                    // do stuff
                    close(sockfd);
                }
                if (childpid > 0){
                    //still the parent process
                    pid_t currentPID = getpid();
                    isChild = FALSE;

                    log_printf(SLL_INFO|SLC_PROCESSDISPATCHING, "SockethandlerPID:%d Mode: %d] Current PID is %d \n", currentPID, currentProcessType, currentPID);
                }
            }

        }
        if (currentProcessType == childinteraction){
            pid_t currentChildPID = getpid();

            /*//create segment & set permissions
            if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, 0666)) < 0) {
                error(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR when trying to create shared memory\n");
                log_printf(SLC_DEBUG, "STATUS B18\n");
                sem_wait(semStatusVariables);
                log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
                mainSharedVariables->sv_gameLevel = 0;
                sem_post(semStatusVariables);
                log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                log_printf(SLC_DEBUG, "STATUS A18\n");
                break;
            }
            //attach segment to data space
            mainSharedVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);*/

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
                        doHELLO(&returnAction);
                        // Manually triffer sending the nexcessary SIZE command

                        // change struct to string
                        encode(&returnAction, replyMessage);

                        //write response to socket
                        length = write(newsockfd, replyMessage, 18);
                        if (length < 0) error(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR writing to socket");

                        doSTART(mainSharedVariables, &returnAction);
                        break;
                    case TAKE:
                        doTAKE(mainSharedVariables, currentAction.iParam1, currentAction.iParam2, currentAction.sParam1, &returnAction);
                        break;
                    case STATUS:
                        doSTATUS(currentAction.iParam1, currentAction.iParam2, &returnAction);
                        break;
                    case END:
                        doEND(mainSharedVariables, currentAction.sParam1, &returnAction);
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
            //sem_post(semStatusVariables);
            char logstring[25];
            sprintf(logstring, "[GameplayPID:%d Mode: %d]",currentPID, currentProcessType);


            printPlayers(mainSharedVariables, logstring);
            printPlayfield();

            sleep(2);

            log_printf(SLC_DEBUG, "STATUS B13\n");
            sem_wait(semStatusVariables);
            log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
            if (mainSharedVariables->sv_numberOfPlayers >= (FIELDSIZE/2) && mainSharedVariables->sv_gameLevel == 1){
                mainSharedVariables->sv_gameLevel=2;
                sem_post(semStatusVariables);
                log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                log_printf(SLC_DEBUG, "STATUS A13A\n");

                log_printf(SLL_INFO|SLC_GAMEPLAY, "Game could be started");
            }
            else {
                log_printf(SLL_INFO | SLC_GAMEPLAY, "Current Shared Memory: GameLevel: %d, Fieldsize: %d, NumberOfPlayers: %d, NumberOfPlayerNames: %d\n",
                   mainSharedVariables->sv_gameLevel, FIELDSIZE,
                   mainSharedVariables->sv_numberOfPlayers, mainSharedVariables->sv_numberOfPlayerNames);
                sem_post(semStatusVariables);
                log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
                log_printf(SLC_DEBUG, "STATUS A13B\n");
            }
            //3. are we done yet
        }
        log_printf(SLC_DEBUG, "STATUS B14B\n");
        sem_wait(semStatusVariables);
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
    }
    sem_post(semStatusVariables);
    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
    log_printf(SLC_DEBUG, "STATUS A14B\n");


    if (currentProcessType == gameplay) {
        // tell all forks to exit loop
        log_printf(SLC_DEBUG, "STATUS B19\n");
        sem_wait(semStatusVariables);
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
        mainSharedVariables->sv_gameLevel = 0;
        sem_post(semStatusVariables);
        log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
        log_printf(SLC_DEBUG, "STATUS A19\n");

        pid_t child_pid, wpid;
        int status = 0;

        while ((wpid = wait(&status)) > 0)
        {
            log_printf(SLL_INFO | SLC_GAMEPLAY,"Exit status of forked process %d was %d (%s)\n", (int)wpid, status, (status > 0) ? "accept" : "reject");
        }

        cleanUpGameplay(mainSharedVariables, sockfd);
    }

    if (currentProcessType == sockethandler)
        cleanUpSocketHandler(newsockfd);
    if (currentProcessType == childinteraction)
        cleanUpChildInteraction(mainSharedVariables);

    return 0;
}

