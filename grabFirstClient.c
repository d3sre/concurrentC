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
#include "grabProtocoll.h"


/*
 *  Game default parameters, might be exported or serialized
 */
const char *CLIENTNAME = "alice";

int gameon = 0;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void parse(const char* cmd){

    char mainCommand[255], proto1st[255], proto2nd[255], proto3rd[255], proto4th[255];

    sscanf(cmd, "%s %s %s %s", proto1st, proto2nd, proto3rd, proto4th);

    if(strcmp(proto1st,"SIZE") == 0)
        SIZE(atoi(proto2nd));
    else if(strcmp(proto1st,"NACK") == 0)
        NACK();
    else if(strcmp(proto1st,"START") == 0)
        START();
    else if(strcmp(proto1st,"TAKEN") == 0)
        TAKEN();
    else if(strcmp(proto1st, "INUSE") == 0)
        INUSE();
    else if(strcmp(proto1st, "END") == 0)
        gameon = 0;
    else
        printf("User %s is owner\n", proto1st);



//    printf("entered parser-2 %s\n", proto1st);
//    printf("entered parser-2 %s\n", proto2nd);
//    printf("entered parser-2 %s\n", proto3rd);
//    printf("entered parser-2 %s\n", proto4th);

}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
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
    while(gameon == 1) {
        printf("Please enter the message: ");
        //clean buffer
        bzero(buffer, 256);
        //gets string from stream stdin, terminated with newline \n
        fgets(buffer, 255, stdin);
        //writes lenght of buffer from buffer to sockfd, n = numbers of written or -1 for error
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");
        //clean buffer
        bzero(buffer, 256);
        // read 255 bytes from sockfd into buffer
        n = read(sockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        //print buffer, terminated by newline \n
        printf("Buffer: %s\n", buffer);
        parse(buffer);
        printf("Gameon: %d", gameon);
    }
    close(sockfd);
    return 0;
}
