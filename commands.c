#include "commands.h"
#include "network.h"
#include "utils.h"
#include "mailsOperation.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <sys/socket.h>

// log into the server
void login_to_imap_server(int sockfd, const char* username, const char* password) {
    char sendBuffer[BUFFER_SIZE];
    char recvBuffer[BUFFER_SIZE];
    // send LOGIN commend
    snprintf(sendBuffer, BUFFER_SIZE, "A01 LOGIN \"%s\" \"%s\"\r\n", username, password);
    write_message(sockfd, sendBuffer);
    receive_message(sockfd,recvBuffer, sizeof(recvBuffer));
    if (strstr(recvBuffer, "\r\n")) {
        //printf("%s", recvBuffer);
        if (strstr(recvBuffer, "OK")) {
            //printf("Login successful\n");
        } else {
            printf("Login failure\n");
            close(sockfd);
            exit(3);
        }
    } else {
        printf("Login failure\n");
        close(sockfd);
        exit(3);
    }
}

// select folder which want to read from
void select_folder(int sockfd, const char* folder){    
    char sendBuffer[BUFFER_SIZE];
    char recvBuffer[BUFFER_SIZE];
    int n;
    snprintf(sendBuffer, BUFFER_SIZE, "A02 SELECT \"%s\"\r\n", folder);
    write_message(sockfd, sendBuffer);
 
    while ((n = read(sockfd, recvBuffer, BUFFER_SIZE - 1)) > 0) {
        recvBuffer[n] = '\0';  
        // Check the response for '* OK' or 'A02 OK' (indicating success)
        if (strstr(recvBuffer, "OK")) {
            //printf("Folder selected successfully.\n");
            //printf("%s", recvBuffer);
            break;
        } else { 
            printf("Folder not found\n");
            close(sockfd);
            exit(3);
        }
        memset(recvBuffer, 0, BUFFER_SIZE); 
    } 
}

//  fetch the email, If the message sequence number was not specified on the command line, then fetch the last 
// added message in the folder
void retrieve_message(int sockfd, char *messageNum) {
    char sendBuffer[BUFFER_SIZE];
    snprintf(sendBuffer, BUFFER_SIZE, "A03 FETCH %s BODY.PEEK[]\r\n", messageNum);
    write_message(sockfd, sendBuffer);

    int mail_size = fetch_mail_size(sockfd);
    char *mail_content = malloc(sizeof(char) * (mail_size + END_OF_STRING));
    assert(mail_content);
    memset(mail_content, 0, mail_size + END_OF_STRING); // clear memory
    fetch_mail_content(sockfd, mail_content, mail_size);
    printf("%s", mail_content);
    free(mail_content);
}

// change the structure of retrieve_message
void parse_message(int sockfd, char *messageNum){
    char sendBuffer[BUFFER_SIZE];
    char recvBuffer[BUFFER_SIZE * BUFFER_NOTENOUGH];
    char *headers[] = {"From", "To", "Date", "Subject"};
    int num_headers = 4;
    
    for (int i=0; i<num_headers; i++){
        //printf("header is %s\n", headers[i]);
        snprintf(sendBuffer, BUFFER_SIZE, "A03 FETCH %s BODY.PEEK[HEADER.FIELDS (%s)]\r\n", messageNum, headers[i]);
        // Send the command to the server  
        //int sendResult =write(sockfd, sendBuffer, strlen(sendBuffer));
        write_message(sockfd, sendBuffer);
        ssize_t bytesReceived = receive_message(sockfd,recvBuffer, sizeof(recvBuffer));
        recvBuffer[bytesReceived] = '\0';

        if (strstr(recvBuffer, "NO") || strstr(recvBuffer, "BAD")) {
            printf("Message not found\n");
            close(sockfd);
            exit(3);
        }
        //printf("%s", recvBuffer);
        print_parse_info(recvBuffer, headers[i]);
    }
}
// checks the MIME and content type of a message by its number and handles the message accordingly.
void find_mime(int sockfd, char *messageNum) {
    char mime_info[BUFFER_SIZE], content_info[BUFFER_SIZE];
    char media_type[BUFFER_SIZE], boundary_name[BUFFER_SIZE];
    // Retrieves and extracts the MIME version from the message headers
    checkHeaderField(sockfd, messageNum, "mime-version", mime_info);
    char *mime_version = extract_header_content(mime_info, DEFAULT_STR);
    // Retrieves and extracts the content type from the message headers
    checkHeaderField(sockfd, messageNum, "content-type", content_info);
    char *content_type = extract_header_content(content_info, DEFAULT_STR);
    parseContentType(content_type, media_type, boundary_name);

    // Checks if the MIME version and media type are as expected, exits if not
    if (strcmp(mime_version, DEFAULT_VERSION) != 0 || strcmp(media_type, DEFAULT_MEDIA_TYPE) != 0) {
        printf("Top message doesn't match\n");
        exit(4);
    }

   // Prepares boundary strings for MIME multipart parsing
    char startBoundary[strlen(boundary_name) + BUFFER_SIZE];
    char endBoundary[strlen(boundary_name) + BUFFER_SIZE];
    sprintf(startBoundary, "--%s\r\n", boundary_name);
    sprintf(endBoundary, "--%s--\r\n", boundary_name);

    // Requests the entire message body for parsing
    char sendBuffer[BUFFER_SIZE];
    snprintf(sendBuffer, BUFFER_SIZE, "A03 FETCH %s BODY.PEEK[]\r\n", messageNum);
    write(sockfd, sendBuffer, strlen(sendBuffer));

   // Retrieves and processes the mail content
    int mail_size = fetch_mail_size(sockfd);
    char *mail_content = malloc(sizeof(char) * (mail_size + 1));
    assert(mail_content);
    memset(mail_content, 0, mail_size + END_OF_STRING);
    fetch_mail_content(sockfd, mail_content, mail_size);

    // Splits the mail content into lines for easier processing
    int line_count = 0;
    char **lines = split_content_into_lines(mail_content, &line_count);

    // Finds the indices of the body start and end in the message lines
    int body_start_index = find_boundary_index(lines, line_count, startBoundary);
    int body_end_index = find_boundary_index(lines, line_count, endBoundary);

    // Determines the start index of the actual content within the boundaries
    int content_start_index = find_content_start_index(lines, line_count, body_start_index, body_end_index);
    // Prints the content within the specified range
    print_content(lines, content_start_index, body_end_index, startBoundary);

    // Cleans up allocated memory for lines and the mail content
    free_lines(lines, line_count);
    free(mail_content);
}

// print to stdout the subject lines of all the emails in the specified folder, sorted by message sequence number
void list_subjects(int sockfd, const char* folder) {
    char sendBuffer[BUFFER_SIZE];
    char recvBuffer[BUFFER_SIZE * BUFFER_NOTENOUGH]; 

    snprintf(sendBuffer, BUFFER_SIZE, "A03 SEARCH ALL\r\n");
    write_message(sockfd, sendBuffer);
    ssize_t bytesReceived = receive_message(sockfd,recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // store the length of first line, make it be a string.
    int firstLineEnd = 0;
    for (int i = 0; i < bytesReceived - 1; i++) {
        if (recvBuffer[i] == '\r' && recvBuffer[i + 1] == '\n') {
            firstLineEnd = i;
            break;
        }
    }

    if (!firstLineEnd) {
        // print error message
        fprintf(stderr, "Improper response format\n");
        // ?
        exit(2);
    }

    recvBuffer[firstLineEnd] = '\0';
    char destBuffer[bytesReceived];
    strcpy(destBuffer, recvBuffer);  // Make a copy for * SEARCH 1, 2, 3
    char *ptr = strstr(destBuffer, "* SEARCH");
    if (!ptr) {
        // If "* SEARCH" is not found, there is no mail in the mailbox
        exit(0);  
    }

    const char *delim = " ";
    char* token = strtok(ptr, delim);
    token = strtok(NULL, delim);  //  skip "SEARCH"
    if (!token) {
        // if no sequence num, the mailbox is empty
        exit(0);
    }
    while ((token = strtok(NULL, delim)) != NULL){
        // printf("token is %s\n", token);
        snprintf(sendBuffer, BUFFER_SIZE, "A04 FETCH %s (BODY.PEEK[HEADER.FIELDS (SUBJECT)])\r\n",token);
        write_message(sockfd, sendBuffer);
        ssize_t bytesReceived = receive_message(sockfd,recvBuffer, sizeof(recvBuffer));
        recvBuffer[bytesReceived] = '\0'; 

        char *subject_start = strstr(recvBuffer, "Subject:");
        if (subject_start) {
            subject_start += SUBJECT_STRLEN; // skip "Subject:" 
            char *subject_end = strstr(subject_start, "\r\n");
            while (subject_end && (*(subject_end + SUBJECT_END2) == ' ' || *(subject_end + SUBJECT_END2) == '\t')) {
                // Continue to the end of the folded lines
                subject_end = strstr(subject_end + SUBJECT_END2, "\r\n");
            }
            if (subject_end) {
                *subject_end = '\0';
            }
            unfold_header(subject_start); // Handle possible multi-line topics
            printf("%s: %s\n", token, subject_start);
            // printf("%s: %s\n", token, subject);
        } else {
            printf("%s: <No subject>\n", token);
        }
    }
}