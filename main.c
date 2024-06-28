// A simple email client program for IMAP server
// To compile: gcc client.c -o client

#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include <sys/socket.h>

#include "network.h"
#include "commands.h"
#include "utils.h"
#include "mailsOperation.h"

#define COMMEND_NUMBER_ATLEAST 6
#define COMMEND_NUMBER_ATMOST 11
#define BUFFER_SIZE 1024
#define PORT_WITH_TSL "993"
#define DEFAULT_PORT "143"

int main(int argc, char *argv[]) {
    int set_tsl = 0;
    char *username = NULL, *password = NULL, *command = NULL, *server_name = NULL; 
    char *messageNum = NULL, *folder_name = "INBOX"; // the default value is INBOX

    if (argc < COMMEND_NUMBER_ATLEAST || argc > COMMEND_NUMBER_ATMOST) { 
        fprintf(stderr,"usage %s hostname port username password\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            // username = argv[i + 1];
            char *unsanitized_username =  argv[i + 1];
            if (!validate_username(unsanitized_username)) {
                fprintf(stderr, "Invalid username format.\n");
                //printf("Invalid username format.\n");
                exit(EXIT_FAILURE);
            }
            username = sanitize_input(unsanitized_username);
            i += 2; 
            continue;
        }
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            // password = argv[i + 1];
            char *unsanitized_password =  argv[i + 1];
            if (!validate_password(unsanitized_password)) { 
                fprintf(stderr, "Invalid password format.\n");
                exit(EXIT_FAILURE);
            }
            password = sanitize_input(unsanitized_password);
            i += 2; 
            continue;
        }
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            folder_name = argv[i + 1];
            // if (!validate_input(folder_name)) {
            //     fprintf(stderr, "Invalid folder name format.\n");
            //     exit(EXIT_FAILURE);
            // }
            i += 2; 
            continue;
        }
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            messageNum = argv[i + 1];
            if (!validate_message_number(messageNum)) {
                fprintf(stderr, "Invalid message number format.\n");
                exit(EXIT_FAILURE);
            }
            i += 2; 
            continue;
        }
        if (strcmp(argv[i], "-t") == 0) {
            set_tsl = 1;
            i += 1; 
            continue;
        }
        i++; // Only increments i if the current argument does not match any known options
    }

    // read commnad and server name from command line
    for (int i = 1; i < argc; i++){
        // if current string not start with '-'
        // check the previous string, if is also not start with '-', then it should be command or server name

        // if current is the first args
        if (argv[i][0] != '-' && i==1){
            command = argv[i];
        }
        // if not the first args
        if (argv[i][0] != '-' && argv[i-1][0] != '-'){
            // if command is NULL then the args is command, otherwise is server name
            if (command == NULL){
                command = argv[i];
            }else if (server_name == NULL){
                server_name = argv[i];
            }else {
                printf("Invalid arguments\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (username == NULL || password == NULL || command == NULL || server_name == NULL){
        fprintf(stderr, "Do not have enough infomation\n");
        exit(1);
    }

    //printf("username is: %s \npassword is: %s \n", username, password);

    if(messageNum == NULL){
        messageNum = "*"; // indicate last email's sequence number
    }

    // create a clinet socket
    int sockfd = client_socket(server_name, set_tsl);
    // printf("the socket id is %d\n", sockfd);
    if (sockfd < 0){
        fprintf(stderr, "Failed to connect.\n");
        exit(EXIT_FAILURE);
    } else {  // if connection successful, receive greeting messgae
        char recvBuffer[BUFFER_SIZE];
        receive_message(sockfd,recvBuffer, sizeof(recvBuffer));
    }

    // log into IMAP server
    login_to_imap_server(sockfd, username, password);

    // After login successfully, select the folder we want.
    select_folder(sockfd, folder_name);

    // start handle comment
    if (strcmp(command, "retrieve") == 0){
        retrieve_message(sockfd, messageNum);
    }
    else if (strcmp(command, "parse") == 0){
        parse_message(sockfd, messageNum);
    }
    else if (strcmp(command, "list") == 0){
        list_subjects(sockfd, folder_name);
    }
    else if (strcmp(command, "mime") == 0){
        find_mime(sockfd, messageNum);
    } else{
        fprintf(stderr, "Invalid command\n");
        exit(1);
    }

    free(username);
    free(password);

    close(sockfd);
    return 0;
}