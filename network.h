#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>

#define BUFFER_SIZE 1024
#define PORT_WITH_TSL "993"
#define DEFAULT_PORT "143"

// create socket
int client_socket(char* serverName, int set_tsl);
// write data from sendBuffer
int write_message(int sockfd,const char *sendBuffer);
// send data to server
ssize_t receive_message(int sockfd, char *recvBuffer, size_t bufferSize);
// read response function
void read_response(int sockfd);

#endif