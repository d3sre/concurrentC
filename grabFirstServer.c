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
#include "grabProtocoll.h"

/*
 *  Game default parameters, might be exported or serialized
 */

#define MAXPLAYER 2 //value for n, start when n/2 reached


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void parse(const char* cmd){
    //printf("entered parser");

    char mainCommand[255], proto1st[255], proto2nd[255], proto3rd[255], proto4th[255];


    sscanf(cmd, "%s %s %s %s", proto1st, proto2nd, proto3rd, proto4th);

    if(strcmp(proto1st,"HELLO") == 0)
        HELLO();
    else if(strcmp(proto1st,"TAKE") == 0)
        TAKE(atoi(proto2nd), atoi(proto3rd), proto4th);
    else if(strcmp(proto1st,"STATUS") == 0)
        STATUS(atoi(proto2nd), atoi(proto3rd));
        else
            printf("Command wasn't recognised\n");



    printf("entered parser-2 %s\n", proto1st);
    printf("entered parser-2 %s\n", proto2nd);
    printf("entered parser-2 %s\n", proto3rd);
    printf("entered parser-2 %s\n", proto4th);

}

int main(int argc, char *argv[])
{
    //### section: setup server and socket ###
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];

    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
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

    //### section: receiving input from client ###
    //open new socket and return file descriptor
    // this is to enable communication with client, separate socket with sockfd and client address, needs to happen for every new client connection
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
    // clear buffer
    bzero(buffer,256);
    //read newsockfd to buffer
    n = read(newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    parse(buffer);
    //print received message from buffer
    printf("Here is the message: %s\n",buffer);
    //write response to socket
    n = write(newsockfd,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");

    //### section: cleanup ###
    close(newsockfd);
    close(sockfd);
    return 0;
}
