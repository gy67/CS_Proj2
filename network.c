#include "network.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <sys/socket.h>
// create socket 
int client_socket(char* serverName, int set_tsl){
    int sockfd, rv;
    struct addrinfo hints, *servinfo, *p;
    char port[6]; // store port number
    // set port number with default or with specified
    if (set_tsl == 1){
        strcpy(port, PORT_WITH_TSL); 
    }else{
        strcpy(port, DEFAULT_PORT); // The default port number is 143
    }
    // 	 Create address
    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM; // TCP as transport protocol
    hints.ai_family = AF_UNSPEC; // Use AF_INET6 to force IPv6 or AF_INET for IPv4 AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // TCP as transport protocol
    rv = getaddrinfo(serverName, port, &hints, &servinfo);
    if (rv != 0){  // fail
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }
    // Loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) { 
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {   // fail to connect
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }
        break; // if we get here, we must have connected successfully
    }
    if (p == NULL) {
        // Loop through all results and couldn't connect
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo); // free memory that allocate to store address info
    return sockfd;
}

// write data from sendBuffer
int write_message(int sockfd,const char *sendBuffer){
    int bytesSend = write(sockfd, sendBuffer, strlen(sendBuffer));
    if (bytesSend < 0) {
        perror("write failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// receive message from server 
ssize_t receive_message(int sockfd, char *recvBuffer, size_t bufferSize){
    ssize_t bytesReceived = recv(sockfd, recvBuffer, bufferSize - 1, 0);
    if (bytesReceived < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    } 
    return bytesReceived;

}

// read response function
void read_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int n;
    int messageFound = 0; 
    while ((n = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[n] = '\0';
        // printf("%s", buffer); 
        if (strstr(buffer, "OK") || strstr(buffer, "NO")) {
            // check whether you received a response indicating that the message was not found
            if (strstr(buffer, "NO") && strstr(buffer, "not found")) {
                fprintf(stderr, "Message not found\n");
                exit(3); 
            }
            messageFound = 1;
            break; 
        }
    }

    if (!messageFound) {
        fprintf(stderr, "Message not found\n");
        exit(3);
    }
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(EXIT_FAILURE);
    }

    if (messageFound) {
        exit(0);  
    }
}