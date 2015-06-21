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

char replyMessage[256];

// Global defaults
#define MAX_CLIENTS 256
#define MAX_PLAYER_NAME_LENGTH 243 // Including ending '\0' -> STATUS messages is the longest, using 13 positions if biggest number for x and y has 3 numbers 256-13=243

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
    int sv_winnerID;
};

int FIELDSIZE = -1;
int CHECKFREQUENCY = 2;

static volatile int keepRunning = 1;
enum processType currentProcessType;

void  intHandler(int sig)
{
    log_printf(SLL_INFO,"\n");
    log_printf(SLL_INFO,"=== Ctrl-C was hit ... cleaning up ===\n");
    log_printf(SLL_INFO,"\n");
    keepRunning = 0;
}



//void error(int mask_flag, const char *msg)
//{
//    log_printf(mask_flag,msg);
//    keepRunning = 0;
//}

void printPlayers(struct sharedVariables *shmStatusVariables){
    int i;

    sem_wait(semPlayerList);
    sem_wait(semStatusVariables);


    log_printf(SLL_INFO|SLC_GAMEPLAY, "PlayerList: Number of players: %d / player names: %d\n", shmStatusVariables->sv_numberOfPlayers,shmStatusVariables->sv_numberOfPlayerNames);
    log_printf(SLL_INFO|SLC_GAMEPLAY, "PlayerList:\n");

    for (i = 0; i < shmStatusVariables->sv_numberOfPlayerNames; i++) {
        log_printf(SLL_INFO|SLC_GAMEPLAY, "- ID: %d, Name: %s\n", i, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]);
    }

    sem_post(semStatusVariables);
    sem_post(semPlayerList);
}

int checkPlayfield(){
    log_printf(SLL_INFO|SLC_GAMEPLAY, "Playfield:\n");
    int x;
    int y;
    int winner;
    // Lock all semaphores for playfield elementst
    int i;
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sem_wait(semPlayfieldElement[i]);
    }

    int index;
    winner = shmPlayfield[0];

    for(y=0; y<FIELDSIZE;y++){
        log_printf(SLL_INFO|SLC_GAMEPLAY|SLC_RELEASE,"");
        for(x=0; x< FIELDSIZE; x++) {
            index = FIELDSIZE*y+x;

            if(shmPlayfield[index] != winner){
                winner = -1;
            }
            if(shmPlayfield[index] == -1) {
                log_appendprintf(SLL_INFO|SLC_GAMEPLAY,"-");
            }
            else {
                log_appendprintf(SLL_INFO|SLC_GAMEPLAY,"%d",shmPlayfield[index]);
            }

        }
        log_appendprintf(SLL_INFO|SLC_GAMEPLAY,"\n");
    }
    //unlock all playfield element semaphores
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sem_post(semPlayfieldElement[i]);
    }

    return winner;

}

void cleanUpGameplay(struct sharedVariables *shmStatusVariables) {
    int i;
    char shmmnPlayfieldElementName[256];

    log_printf(SLL_DEBUG|SLC_GAMEPLAY, "In cleanUpGameplay function\n");

    // Close semaphores
    sem_close(semStatusVariables);
    sem_unlink(SHMSN_STATUS_VARIABLES);
    sem_close(semPlayerList);
    sem_unlink(SHMSN_PLAYER_LIST);

    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sem_close(semPlayfieldElement[i]);
        sprintf(shmmnPlayfieldElementName, "%s%d", SHMSN_PLAYFIELD_ELEMENT_PREFIX, i);
        sem_unlink(shmmnPlayfieldElementName);
    }
    free(semPlayfieldElement);

    // Cleanup shared memeory
    shmdt(shmPlayerList);
    shmPlayerList = NULL;
    shmctl(shmPlayerListID, IPC_RMID, NULL);

    // Cleanup playfield in shared memory
    shmdt(shmPlayfield);
    shmPlayfield = NULL;
    shmctl(shmPlayfieldID, IPC_RMID, NULL);

    shmdt(shmStatusVariables);
    shmStatusVariables = NULL;
    shmctl(sharedMemIDStruct, IPC_RMID, NULL);

    // Shutdown logger
    shutdown_logger();
}

void cleanUpSocketHandler(int socket) {
    log_printf(SLL_DEBUG|SLC_GAMEPLAY, "In cleanUpSocketHandler function with socket id: %d\n", socket);

    // Close socket
    close(socket);
    close(socket+1); // for listening port of possible other connections

    // Cleanup shared memeory
    shmdt(shmPlayerList);
    shmPlayerList = NULL;
    shmctl(shmPlayerListID, IPC_RMID, NULL);

    // Shutdown logger
    shutdown_logger();
}

void cleanUpChildInteraction(struct sharedVariables *shmStatusVariables,int socket) {
    log_printf(SLL_DEBUG|SLC_GAMEPLAY, "In cleanUpChildInteraction function with socket id: %d\n", socket);

    // Close socket
    close(socket);

    // Cleanup shared memeory
    shmdt(shmPlayerList);
    shmPlayerList = NULL;
    shmctl(shmPlayerListID, IPC_RMID, NULL);

    shmdt(shmStatusVariables);
    shmStatusVariables = NULL;
    shmctl(sharedMemIDStruct, IPC_RMID, NULL);

    shmdt(shmPlayfield);
    shmPlayfield = NULL;
    shmctl(shmPlayfieldID, IPC_RMID, NULL);

    // Shutdown logger
    shutdown_logger();
}

void addPlayer(struct sharedVariables *shmStatusVariables, const char* playername) {
    log_printf(SLL_DEBUG|SLC_GAMEPLAY, "in addPlayer function\n");

    // Wait for necessary locks
    sem_wait(semPlayerList);;
    sem_wait(semStatusVariables);

    strcpy(&shmPlayerList[shmStatusVariables->sv_numberOfPlayerNames*MAX_PLAYER_NAME_LENGTH], playername);
    shmStatusVariables->sv_numberOfPlayerNames++;

    sem_post(semStatusVariables);
    sem_post(semPlayerList);
}

_Bool existPlayer(struct sharedVariables *shmStatusVariables, const char* playername) {
    log_printf(SLL_DEBUG|SLC_GAMEPLAY, "in existPlayer function\n");

    int i;
    _Bool result = FALSE;

    // Wait for necessary locks
    sem_wait(semPlayerList);
    sem_wait(semStatusVariables);
    log_printf(SLL_DEBUG|SLC_DEBUG_SEM_STATUS, "Number Players: %d\n", shmStatusVariables->sv_numberOfPlayerNames);

    for (i = 0; i < shmStatusVariables->sv_numberOfPlayerNames; i++) {
        if (strcmp(playername, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]) == 0) {
            result = TRUE;
            log_printf(SLL_DEBUG|SLC_DEBUG_SEM_STATUS, "Player found at ID %d\n", i);
            break;
        }
    }


    sem_post(semStatusVariables);
    sem_post(semPlayerList);

    return result;
}

int getPlayerID(struct sharedVariables *shmStatusVariables, const char* playername){
    log_printf(SLL_DEBUG|SLC_GAMEPLAY, "getPlayerID function\n");

    int i;
    int id = -1;

    if (!existPlayer(shmStatusVariables,playername))
        return id;

    // Wait for necessary locks
    sem_wait(semPlayerList);
    sem_wait(semStatusVariables);

    for(i = 0; i< shmStatusVariables->sv_numberOfPlayerNames; i++) {
        if (strcmp(playername, &shmPlayerList[i*MAX_PLAYER_NAME_LENGTH]) == 0) {
            id = i;
            break;
        }
    }

    sem_post(semStatusVariables);
    sem_post(semPlayerList);

    return id;
}


_Bool doHELLO(struct action* returnAction) {
    log_printf(SLL_DEBUG|SLC_SOCKETCOMMUNICATION, "HELLO received - playfield size defined is: %d\n", FIELDSIZE);

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

_Bool doTAKE(struct sharedVariables *shmStatusVariables, int x, int y, char *playerName, struct action* returnAction) {
    log_printf(SLL_DEBUG | SLC_SOCKETCOMMUNICATION, "TAKE received on x:%d y: %d by: %s\n", x, y, playerName);

    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || playerName == NULL || returnAction == NULL)
        return FALSE;

    int index = y*FIELDSIZE+x;

    if (!existPlayer(shmStatusVariables,playerName)) {
        addPlayer(shmStatusVariables, playerName);
    }

    if (sem_trywait(semPlayfieldElement[index])== 0) {
        shmPlayfield[index] = getPlayerID(shmStatusVariables,playerName);
        sem_post(semPlayfieldElement[index]);

        returnAction->cmd = TAKEN;

    }
    else {
        returnAction->cmd = INUSE;
    }

    return TRUE;

}

_Bool doSTATUS(int x, int y, struct action* returnAction) {
    log_printf(SLL_DEBUG | SLC_SOCKETCOMMUNICATION, "STATUS x:d% y:%d\n",x,y);
    if (x< 0 || x>=FIELDSIZE || y<0 || y>= FIELDSIZE || returnAction == NULL){
        return FALSE;
    }

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
        sem_wait(semPlayerList);
        strcpy(returnAction->sParam1, &shmPlayerList[clientid*MAX_PLAYER_NAME_LENGTH]);
        sem_post(semPlayerList);
    }
    return TRUE;
}

//_Bool doEND(struct sharedVariables *shmStatusVariables, char *playerName, struct action* returnAction) {
//    log_printf(SLL_INFO | SLC_SOCKETCOMMUNICATION|SLC_RELEASE, "END winner: %s\n", playerName);
//
//    if (playerName == NULL|| returnAction == NULL)
//        return FALSE;
//    log_printf(SLC_DEBUG, "STATUS B10\n");
//    sem_wait(semStatusVariables);
//    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables LOCKED\n");
//    shmStatusVariables->sv_gameLevel = 0;
//    sem_post(semStatusVariables);
//    log_printf(SLC_DEBUG|SLC_DEBUG_SEM_STATUS, "Status variables UNLOCKED\n");
//    log_printf(SLC_DEBUG, "STATUS A10\n");
//    returnAction->cmd = END;
//    strcpy(returnAction->sParam1, playerName);
//
//    return TRUE;
//}

_Bool doSTART(struct sharedVariables *shmStatusVariables, struct action* returnAction) {
    //create segment & set permissions
    while(TRUE) {
        sem_wait(semStatusVariables);
        if (shmStatusVariables->sv_gameLevel != 2) {
            log_printf(SLL_DEBUG| SLC_CHILDINTERACTION, "Game is not yet started\n");
        }
        else {
            //start the game
            log_printf(SLL_INFO|SLC_GAMEPLAY, "Game level was raised\n");

            log_printf(SLL_INFO|SLC_SOCKETCOMMUNICATION|SLC_RELEASE, "START all\n");

            returnAction->cmd = START;
            sem_post(semStatusVariables);
            break;
        }
        sem_post(semStatusVariables);
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
    // error output directly to stderr as no log yet available (logging level only received during startupparameter check)
    portno = atoi(argv[1]);
    if(atoi(argv[2]) >=4) {
        FIELDSIZE = atoi(argv[2]);
    }
    else {
        fprintf(stderr,"Usage: Fieldsize must be equal or bigger than 4\n");
        keepRunning=0;
    }


    if((atoi(argv[3]) >= 1) && (atoi(argv[3]) <= 30)) {
        CHECKFREQUENCY = atoi(argv[3]);
    }
    else {
        fprintf(stderr,"Usage: check interval must be 1 <= y <= 30\n");
        keepRunning=0;
    }

    if(argc == 5){
        if((strcmp(argv[4],"RELEASE")==0) ||(strcmp(argv[4],"DEBUG")==0)){
            strncpy(loglevel, argv[4], 10);
        }
        else {
            strncpy(loglevel,"RELEASE",10);
        }
    }
    else {
        strncpy(loglevel,"RELEASE",10);
    }

    if ((argc != 4) && (argc != 5)) {
        fprintf(stderr,"Usage: grabFirstServer <portnumber> <fieldSize> <checkfrequency> [RELEASE | DEBUG]\n");
        keepRunning=0;
    }

    sprintf(gameplayLogTag, "GAMEPLAY:%d", getpid());
    //startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLC_GAMEPLAY, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    if(strcmp(loglevel,"RELEASE") == 0) {
        startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLL_ERROR |SLL_INFO, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    }
    else {
        startup_logger(gameplayLogTag, SLO_CONSOLE | SLO_FILE, SLL_DEBUG| SLL_ERROR |SLL_INFO| SLC_GAMEPLAY, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
    }

    currentProcessType = gameplay;

    pid_t parentProcess = getpid();



    // Open shared nemory semaphores
    // semaphore for status variables
    if ((semStatusVariables = sem_open(SHMSN_STATUS_VARIABLES, O_CREAT, 0600, 1)) == SEM_FAILED) {
        log_printf(SLL_ERROR | SLC_GENERALERRORS,"ERROR with semaphore initialization for status variables semStatusVariables\n");
        keepRunning = 0;
    }

    // semaphore for player list
    if ((semPlayerList = sem_open(SHMSN_PLAYER_LIST, O_CREAT, 0600, 1)) == SEM_FAILED) {

        log_printf(SLL_ERROR | SLC_GENERALERRORS,"ERROR with semaphore initialization for player list semPlayerList\n");
        keepRunning = 0;
    }



    // semaphores for playfield elementst
    int i;
    char shmmnPlayfieldElementName[256];
    semPlayfieldElement = malloc(FIELDSIZE*FIELDSIZE*sizeof(sem_t));
    for (i = 0; i < FIELDSIZE*FIELDSIZE; i++) {
        sprintf(shmmnPlayfieldElementName, "%s%d", SHMSN_PLAYFIELD_ELEMENT_PREFIX, i);
        if ((semPlayfieldElement[i] = sem_open(shmmnPlayfieldElementName, O_CREAT, 0600, 1)) == SEM_FAILED) {
            log_printf(SLL_ERROR | SLC_GENERALERRORS,"ERROR with semaphore initialization for playfield elements semPlayfieldElement[i]\n");
            keepRunning = 0;
        }
    }

    //shared memory definition, source from http://www.lainoox.com/ipc-shared-memory-c/
    sharedMemSizeStruct = sizeof(struct sharedVariables); //like example from website
    //create segment & set permissions
    if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, IPC_CREAT | 0666)) < 0) {
        log_printf(SLL_ERROR | SLC_GENERALERRORS, "ERROR when trying to create shared memory for status variables sharedMemIDStruct\n");
        keepRunning = 0;
    }


    //attach segment to data space
    struct sharedVariables *shmStatusVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);
    memset(shmStatusVariables,0,sharedMemSizeStruct);

    // Create Player List in shared memory
    shmPlayerListMemSize = MAX_CLIENTS*MAX_PLAYER_NAME_LENGTH*sizeof(char);
    // Create segment & set permissions
    if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, IPC_CREAT | 0666)) < 0) {
        log_printf(SLL_ERROR | SLC_GENERALERRORS, "ERROR when trying to create shared memory for player list shmPlayerListID\n");
        keepRunning = 0;
    }
    // Attach segment to data space
    if ((shmPlayerList = (char*)shmat(shmPlayerListID, NULL, 0)) == (char*)-1) {
        log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach player list segment to data space shmPlayerList\n");
        keepRunning = 0;
    }
    memset(shmPlayerList, 0, shmPlayerListMemSize);


    shmStatusVariables->sv_gameLevel=0;
    shmStatusVariables->sv_numberOfPlayers=0;
    shmStatusVariables->sv_numberOfPlayerNames = 0;
    shmStatusVariables->sv_winnerID = -1;

    // Create Playerfield in shared memory
    shmPlayfieldMemSize = FIELDSIZE*FIELDSIZE*sizeof(int);
    // Create segment & set permissions
    if ((shmPlayfieldID = shmget(SHMK_PLAYFIELD, shmPlayfieldMemSize, IPC_CREAT | 0666)) < 0) {
        log_printf(SLL_ERROR | SLC_GENERALERRORS, "ERROR when trying to create shared memory for playfield\n");
        keepRunning = 0;
    }
    // Attach segment to data space
    if ((shmPlayfield = (int*)shmat(shmPlayfieldID, NULL, 0)) == (int*)-1) {
        log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach segment to data space for playfield\n");
        keepRunning = 0;
    }
    memset(shmPlayfield, -1, shmPlayfieldMemSize);




    //raise game level
    shmStatusVariables->sv_gameLevel=1;
    pid_t childpid, masterpid;
    _Bool isChild = FALSE;


    masterpid = fork();
    if (masterpid < 0) {
        log_printf(SLL_ERROR | SLC_GENERALERRORS, "error on master fork\n");
        keepRunning = 0;
    }
    if (masterpid == 0){
        //entered first child
        currentProcessType = sockethandler;

        char socketHandlerLogTag[256];
        sprintf(socketHandlerLogTag, "SOCKETHANDLER:%d", getpid());
        //startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLC_SOCKETHANDLER, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
        if(strcmp(loglevel,"RELEASE") == 0) {
            startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLL_ERROR |SLL_INFO, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
        }
        else {
            startup_logger(socketHandlerLogTag, SLO_CONSOLE | SLO_FILE, SLL_ERROR | SLL_DEBUG| SLL_INFO| SLC_SOCKETHANDLER, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
        }

        // SOCK_STREAM for TCP, Domain for Internet, protocol chosen automatically
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            log_printf(SLL_ERROR | SLC_GENERALERRORS, "ERROR on opening socket\n");
            keepRunning = 0;
        }
        //set sizeof(serv_addr) of bytes to zero
        bzero((char *) &serv_addr, sizeof(serv_addr));
        //configure serv_addr structure object with server details
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        //bind  socket file descriptor to serv_addr, with length sizeof(serv_addr) and if error print out
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            log_printf(SLL_ERROR | SLC_GENERALERRORS, "ERROR on binding socket to port\n");
            keepRunning=0;
        }
        listen(sockfd,5);
        clilen = sizeof(cli_addr);

        // Get access to Player List in shared memory
        // get segment & set permissions
        if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, 0666)) < 0) {
            log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to access shared memory of playerlist \n");
            keepRunning = 0;
        }
        //attach segment to data space
        if ((shmPlayerList = (char*)shmat(shmPlayerListID, NULL, 0)) == (char*)-1) {
            log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach player list segment to data space \n");
            keepRunning = 0;
        }
    }
    if (masterpid > 0) {
        //still in master PID
        currentProcessType = gameplay;
    }

    //### section: receiving input from client ###
    sem_wait(semStatusVariables);;
    while ((shmStatusVariables->sv_gameLevel >= 1) && (keepRunning == 1)) {
        sem_post(semStatusVariables);

        //printf("I'm in the loop %d\n", getpid());
        //1. wait for new clients
        //open new socket and return file descriptor
        // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
        if (currentProcessType == sockethandler) {
            sem_wait(semStatusVariables);
            if(shmStatusVariables->sv_gameLevel < 3) {
                sem_post(semStatusVariables);
                log_printf(SLL_DEBUG | SLC_SOCKETHANDLER, "SockethandlerPID:%d Mode: %d] Entered Sockethandler interaction loop part\n", getpid(), currentProcessType);

                fd_set rfds;
                struct timeval tv;
                int retval;

                /* Watch stdin (fd 0) to see when it has input. */
                FD_ZERO(&rfds);
                FD_SET(sockfd, &rfds);

                /* Wait up to five seconds. */
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                retval = select(sockfd+1, &rfds, NULL, NULL, &tv);

                if (retval > 0) {


                    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

                    if (newsockfd < 0) {
                        log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR on accept\n");
                    }
                    else {
                        log_printf(SLL_INFO | SLC_PROCESSDISPATCHING | SLC_RELEASE, "SockethandlerPID:%d Mode: %d] New connection was accepted (newsockfd): %d\n", getpid(), currentProcessType, newsockfd);
                        childpid = fork();

                        if (childpid < 0) {
                            log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR on fork\n");

                        }
                        if (childpid == 0) {
                            pid_t currentChildPID = getpid();
                            pid_t parentPID = getppid();


                            char childLogTag[256];
                            sprintf(childLogTag, "CHILDINTERACTION:%d", currentChildPID);
                            //startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_INFO |SLL_ERROR , SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
                            if (strcmp(loglevel, "RELEASE") == 0) {
                                startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_ERROR | SLL_INFO,
                                               SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
                            }
                            else {
                                startup_logger(childLogTag, SLO_CONSOLE | SLO_FILE, SLL_INFO | SLL_ERROR | SLL_DEBUG,
                                               SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
                            }

                            // Get access to Player List in shared memory
                            // get segment & set permissions
                            if ((shmPlayerListID = shmget(SHMK_PLAYER_LIST, shmPlayerListMemSize, 0666)) < 0) {
                                log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to access shared memory for player list shmPlayerListID\n");
                                sem_wait(semStatusVariables);
                                shmStatusVariables->sv_gameLevel = 0;
                                sem_post(semStatusVariables);
                                break;
                            }
                            //attach segment to data space
                            if ((shmPlayerList = (char *) shmat(shmPlayerListID, NULL, 0)) == (char *) -1) {
                                log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to attach player list segment to data space \n");
                                sem_wait(semStatusVariables);
                                shmStatusVariables->sv_gameLevel = 0;
                                sem_post(semStatusVariables);
                                break;
                            }

                            isChild = TRUE;
                            log_printf(SLL_INFO | SLC_PROCESSDISPATCHING, "ChildinteractionPID: %d, Mode: %d, New Child PID is %d with parent %d\n", currentChildPID, currentProcessType, currentChildPID, parentPID);

                            //create segment & set permissions
                            if ((sharedMemIDStruct = shmget(SHMK_STATUS_VARIABLES, sharedMemSizeStruct, 0666)) < 0) {
                                log_printf(SLL_ERROR | SLC_PROCESSDISPATCHING, "ERROR when trying to access shared memory\n");
                                sem_wait(semStatusVariables);
                                shmStatusVariables->sv_gameLevel = 0;
                                sem_post(semStatusVariables);
                                break;
                            }
                            //attach segment to data space
                            shmStatusVariables = (struct sharedVariables *) shmat(sharedMemIDStruct, NULL, 0);
                            sem_wait(semStatusVariables);
                            shmStatusVariables->sv_numberOfPlayers++;
                            log_printf(SLL_DEBUG | SLC_PROCESSDISPATCHING, "ChildinteractionPID:%d Mode : %d] New number of sockets: %d\n", currentChildPID, currentProcessType, shmStatusVariables->sv_numberOfPlayers);
                            sem_post(semStatusVariables);
                            currentProcessType = childinteraction;

                            // do stuff
                            close(sockfd);
                        }
                        if (childpid > 0) {
                            //still the parent process
                            pid_t currentPID = getpid();
                            isChild = FALSE;

                            log_printf(SLL_DEBUG | SLC_PROCESSDISPATCHING, "SockethandlerPID:%d Mode: %d] Current PID is %d \n", currentPID, currentProcessType, currentPID);
                        }
                    }
                }
            }
            else{
                sem_post(semStatusVariables);
                log_printf(SLL_DEBUG | SLC_PROCESSDISPATCHING, "No input received in sockethandler, keepRunning was set to 0\n");
                keepRunning = 0;

            }
        }
        if (currentProcessType == childinteraction){
            pid_t currentChildPID = getpid();

            sem_wait(semStatusVariables);
            if(shmStatusVariables->sv_gameLevel < 3) {
                sem_post(semStatusVariables);
                // clear buffer
                bzero(buffer, 256);
                //check for incoming traffic and if there is, read newsockfd to buffer
                fd_set rfds;
                struct timeval tv;
                int retval;

                /* Watch stdin (fd 0) to see when it has input. */
                FD_ZERO(&rfds);
                FD_SET(newsockfd, &rfds);

                /* Wait up to five seconds. */
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                retval = select(newsockfd+1, &rfds, NULL, NULL, &tv);

                if (retval > 0) {

                    length = read(newsockfd, buffer, 255);
                    if (length > 0) {
                        //error("ERROR reading from socket");

                        // change buffer (string) to struct action
                        decode(buffer, &currentAction);

                        //possible receiving commands
                        switch (currentAction.cmd) {
                            case HELLO:
                                doHELLO(&returnAction);
                                // Manually trigger sending the necessary SIZE command

                                // change struct to string
                                encode(&returnAction, replyMessage);

                                //write response to socket
                                length = write(newsockfd, replyMessage, 18);
                                if (length < 0) log_printf(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR writing to socket when replying to HELLO message");

                                doSTART(shmStatusVariables, &returnAction);
                                break;
                            case TAKE:
                                doTAKE(shmStatusVariables, currentAction.iParam1, currentAction.iParam2,
                                       currentAction.sParam1, &returnAction);
                                break;
                            case STATUS:
                                doSTATUS(currentAction.iParam1, currentAction.iParam2, &returnAction);
                                break;
                            case END:
                                //client is not supposed to send END (protocol)
                                break;
                            default:
                                log_printf(SLL_DEBUG | SLC_CHILDINTERACTION, "ChildinteractionPID:%d Mode : %d] ERROR received action can't be identified\n", currentChildPID, currentProcessType);
                                break;
                        }

                        //print received message from buffer

                        log_printf(SLL_DEBUG | SLC_CHILDINTERACTION, "ChildinteractionPID:%d Mode : %d] Message received: %s\n", currentChildPID, currentProcessType, buffer);

                        // change struct to string
                        encode(&returnAction, replyMessage);

                        //write response to socket
                        length = write(newsockfd, replyMessage, 18);
                        if (length < 0) {
                            log_printf(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR writing to socket\n");
                            keepRunning = 0;
                        }
                    }
                }
                else {
                    log_printf(SLL_DEBUG | SLC_CHILDINTERACTION, "No input received in childinderaction (waitet for 5sec)\n");
                    //close(newsockfd+1);
                }
            }
            else{
                int winner = shmStatusVariables->sv_winnerID;
                sem_post(semStatusVariables);
                log_printf(SLL_DEBUG | SLC_CHILDINTERACTION, "gameLevel >=3 was reached, winnerID is %d\n", winner);

                returnAction.cmd = END;
                sem_wait(semPlayerList);
                strcpy(returnAction.sParam1, &shmPlayerList[winner*MAX_PLAYER_NAME_LENGTH]);
                sem_post(semPlayerList);

                // change struct to string
                encode(&returnAction, replyMessage);

                //write response to socket
                length = write(newsockfd, replyMessage, 18);
                if (length < 0) log_printf(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR writing to socket\n");

                keepRunning = 0;
            }



        }
        if (currentProcessType == gameplay){


            pid_t currentPID = getpid();
            log_printf(SLL_DEBUG|SLC_GAMEPLAY, "GameplayPID: %d, Mode: %d, Entered gameplay interaction loop part\n", currentPID, currentProcessType);


            //2. can we start the game yet

            sem_wait(semStatusVariables);
            if (shmStatusVariables->sv_numberOfPlayers >= (FIELDSIZE/2) && shmStatusVariables->sv_gameLevel == 1){
                shmStatusVariables->sv_gameLevel=2;
                sem_post(semStatusVariables);

                log_printf(SLL_INFO|SLC_GAMEPLAY, "Game could be started\n");
            }
            else {
                log_printf(SLL_DEBUG | SLC_GAMEPLAY, "Current Shared Memory - GameLevel: %d, Fieldsize: %d, NumberOfPlayers: %d, NumberOfPlayerNames: %d\n",
                   shmStatusVariables->sv_gameLevel, FIELDSIZE,
                   shmStatusVariables->sv_numberOfPlayers, shmStatusVariables->sv_numberOfPlayerNames);
                sem_post(semStatusVariables);
            }

            //3. are we done yet
            sem_wait(semStatusVariables);
            if(shmStatusVariables->sv_gameLevel == 2){
                sem_post(semStatusVariables);

                sleep(CHECKFREQUENCY);
                printPlayers(shmStatusVariables);
                int winner = checkPlayfield();
                char winnername[256];
                if(winner != -1){
                    sem_wait(semStatusVariables);
                    shmStatusVariables->sv_gameLevel = 3;
                    shmStatusVariables->sv_winnerID = winner;
                    sem_post(semStatusVariables);
                    sem_wait(semPlayerList);
                    strcpy(winnername, &shmPlayerList[winner*MAX_PLAYER_NAME_LENGTH]);
                    sem_post(semPlayerList);
                    log_printf(SLL_INFO|SLC_GAMEPLAY, "Winner is %d - %s - Game is over\n", winner, winnername);
                    keepRunning = 0;
                }

            }
            else {
                sem_post(semStatusVariables);
            }

        }
        sem_wait(semStatusVariables);
    }
    sem_post(semStatusVariables);


    if (currentProcessType == gameplay) {
        sem_wait(semStatusVariables);
        log_printf(SLL_DEBUG | SLC_GAMEPLAY,"sv_gameLevel is %d\n", shmStatusVariables->sv_gameLevel);
        if(shmStatusVariables->sv_gameLevel !=3) {
            // tell all forks to exit loop
            shmStatusVariables->sv_gameLevel = 0;
            log_printf(SLL_DEBUG | SLC_GAMEPLAY,"Set sv_gameLevel = 0\n");
        }
        sem_post(semStatusVariables);

        pid_t child_pid, wpid;
        int status = 0;

        while ((wpid = wait(&status)) > 0)
        {
            log_printf(SLL_DEBUG | SLC_GAMEPLAY,"Exit status of forked process %d was %d (%s)\n", (int)wpid, status, (status > 0) ? "accept" : "reject");
        }

        cleanUpGameplay(shmStatusVariables);
    }

    if (currentProcessType == sockethandler) {
        sem_wait(semStatusVariables);
        log_printf(SLL_DEBUG | SLC_SOCKETHANDLER,"sv_gameLevel is %d\n", shmStatusVariables->sv_gameLevel);
        if(shmStatusVariables->sv_gameLevel !=3) {
            shmStatusVariables->sv_gameLevel = 0;
            log_printf(SLL_DEBUG | SLC_SOCKETHANDLER,"Set sv_gameLevel = 0\n");

        }
        sem_post(semStatusVariables);
        cleanUpSocketHandler(sockfd);
    }
    if (currentProcessType == childinteraction) {
        log_printf(SLL_DEBUG | SLC_CHILDINTERACTION, "Last parameters before cleanup: sv_gameLevel: %d / keepRunning: %d / newsockfd: %d\n", shmStatusVariables->sv_gameLevel, keepRunning, newsockfd);
        sem_wait(semStatusVariables);
        log_printf(SLL_DEBUG | SLC_SOCKETHANDLER,"sv_gameLevel is %d\n", shmStatusVariables->sv_gameLevel);
        if(shmStatusVariables->sv_gameLevel !=3) {
            sem_post(semStatusVariables);
            returnAction.cmd = END;
            strcpy(returnAction.sParam1, "ABORT");

            // change struct to string
            encode(&returnAction, replyMessage);

            //write response to socket
            length = write(newsockfd, replyMessage, 18);
            if (length < 0) log_printf(SLL_ERROR | SLC_CHILDINTERACTION, "ERROR writing to socket");

        }
        else {
            sem_post(semStatusVariables);
        }

        cleanUpChildInteraction(shmStatusVariables, newsockfd);
    }

    return 0;
}

