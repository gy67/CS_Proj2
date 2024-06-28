#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <sys/socket.h>
#include "mailsOperation.h"
#include "network.h"
#include "utils.h"

// Read the response from the mail server to determine the size of the message
int fetch_mail_size(int sockfd) {
    char c;
    char response_line[BUFFER_SIZE];
    int num_char = 0;
    while (1) {
        read(sockfd, &c, sizeof(char));
        response_line[num_char++] = c;
        if (c == '\n') {
            response_line[num_char] = '\0';
            break;
        }
    }
    // Check the response for the error identifier "NO" or "BAD".
    if (strstr(response_line, "NO") || strstr(response_line, "BAD")) {
        printf("Message not found\n");
        exit(3);
    }
    int mail_size;
    if (sscanf(response_line, "* %*d FETCH (BODY[] {%d}", &mail_size) == 1) {
        return mail_size;
    } else {
        fprintf(stderr, "Failed to extract mail size from response.\n");
        exit(3);
    }
}

// Read the message content according to the size provided
void fetch_mail_content(int sockfd, char *mail_content, int mail_size) {
    int total_bytes_read = 0, result;
    // The data is read until the specified message size is reached.
    while (total_bytes_read < mail_size) {
        result = read(sockfd, mail_content + total_bytes_read, mail_size - total_bytes_read);
        if (result <= 0) {
            perror("Error reading from socket");
            exit(3);
        }
        total_bytes_read += result;
    }
    if (total_bytes_read != mail_size) {
        fprintf(stderr, "Error reading the full content: expected %d, got %d bytes.\n", mail_size, total_bytes_read);
        exit(3);
    }
}

// Extracts everything except the first line from the mail message until it encounters a specific closing tag "\r\n ".
void parse_content(char *message, char *content) {
    char *src = message;  // Source pointer for traversing message
    int first_line_skipped = 0;  // Indicates whether the first row has been skipped
    while (*src != '\0') {
        if (!first_line_skipped) {
            if (*src == '\n') {
                first_line_skipped = 1;  // The tag has skipped the first line
                src++;  // move to this 
            } else {
                src++; 
            }
        } else {
            // check if meeting "\r\n)"
            if (*src == '\r' && strncmp(src, "\r\n)", 3) == 0) {
                break;  
            }
            *content++ = *src++;  // duplicate remain context
        }
    }
    *content = '\0'; 
    //printf("after parse, content is %s", content);
}

// Print the content according to the header information type
void print_parse_info(char *message, char* headers) {
    char content[strlen(message)];
    parse_content(message, content);
    unfold_header(content);  
    if (strlen(content) == 0){
        if (strcmp(headers, "To") == 0) {
            printf("%s:\n", headers);
        } else if (strcmp(headers, "Subject") == 0) {
            printf("%s: <No subject>\n", headers);
        } 
    }else{
        char *info = extract_header_content(content, headers);
        printf("%s: %s\n", headers, info);
    }
}

// Extracts the value of a header field from a content string, skipping spaces if not a Subject header.
char *extract_header_content(char *content, char *headers) {
    char *ptr = content;
    while (*ptr != '\0' && *ptr != ':') {  // Finds the first colon or the end of the string
        ptr++;
    }
    if (*ptr == ':') {  // if founded :
        *ptr = '\0';     // Replaces the colon with a null character, ending the header string
        ptr++;           // Move the character after the colon
        if (*ptr == ' ') {
            ptr++;
        }
        
        if (strcmp(headers, "Subject") != 0){

            // skip the space behind the :
            while (*ptr == ' ') {
                ptr++;
            }
        }
        return ptr; 
    }
    return NULL;  // if no ":"，return NULL
}

// Sends a request to fetch specific header field values of a message and stores the result in 'output'.
void checkHeaderField(int sockfd, char *messageNum, char *field, char *output) {
    char sendBuffer[BUFFER_SIZE];
    char recvBuffer[BUFFER_SIZE];
    char content[BUFFER_SIZE]; 
    snprintf(sendBuffer, BUFFER_SIZE, "A03 FETCH %s BODY.PEEK[HEADER.FIELDS (%s)]\r\n", messageNum, field);

    write_message(sockfd, sendBuffer);
    ssize_t bytesReceived = receive_message(sockfd,recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    if (strstr(recvBuffer, "NO") || strstr(recvBuffer, "BAD")) {
        printf("Message not found\n");
        exit(3);
    }   
    
    // Parse and expand the content
    parse_content(recvBuffer, content);
    unfold_header(content);
    
    strcpy(output, content);
}


void parseContentType(char *content_type, char *mediaType, char *boundary) {
    int inQuotes = 0;
    int typeIndex = 0, boundaryIndex = 0;
    const char *ptr = content_type;
    // Skip leading whitespaces
    while (*ptr && isspace((unsigned char)*ptr)) {
        ptr++;
    }
    // Copy media type until encountering a semicolon
    while (*ptr && *ptr != ';') {
        mediaType[typeIndex++] = *ptr++;
    }
    mediaType[typeIndex] = '\0';
    // Find and copy boundary
    while (*ptr) {
        if (*ptr == '\"') {
            inQuotes = !inQuotes;  // Toggle the status of inQuotes on encountering a quote
            ptr++; // Skip the quote character itself
            continue;
        }
        if (!inQuotes && strncmp(ptr, "boundary=", 9) == 0) {
            ptr += 9;  // Move pointer to start of the boundary value
            // Skip leading whitespace after the '='
            while (*ptr && isspace((unsigned char)*ptr)) {
                ptr++;
            }
            // Check if the boundary is enclosed in quotes
            if (*ptr == '\"') {
                ptr++; // Move past the opening quote
            }
            // Copy characters until encountering a semicolon
            while (*ptr && *ptr != ';' && boundaryIndex < 255) {
                if (*ptr == '\"' && *(ptr + 1) == ';') {
                    break; // Check if the next character is a semicolon and the boundary name ends with a quote
                }
                boundary[boundaryIndex++] = *ptr++; // Record characters to boundary
            }
            break; // Boundary found and copied, exit the loop
        }
        ptr++; // Move to the next character
    }
    // Ensure strings are properly null-terminated
    boundary[boundaryIndex] = '\0';
}

// read a line one time
void read_one_line(int sockfd, char *response_line) {
    char c;
    int num_char = 0;
    while (1) {
        read(sockfd, &c, sizeof(char));
        
        response_line[num_char++] = c;
        if (c == '\n') {
            response_line[num_char] = '\0';
            break;
        }
    }
}

// Helper function to check if a line starts with the specified header
int starts_with_header(char *line, char *header_name) {
    int header_length = strlen(header_name);
    char line_header[header_length + 1];  // Buffer to hold the header part of the line
    // Extract the potential header part up to the colon
    int i;
    for (i = 0; i < header_length && line[i] != ':' && line[i] != '\0'; i++) {
        line_header[i] = line[i];
    }
    line_header[i] = '\0';
    // Convert extracted header to lowercase
    char line_header_lower[header_length + 1];
    to_lower_string(line_header, line_header_lower);
    if (strncmp(line_header_lower, header_name, header_length) == 0 && line[header_length] == ':') {
        return 1; 
    }
    return 0;
}

char** split_content_into_lines(char* content, int* line_count) {
    char* delim = "\r\n";
    int count = 0;
    char* tmp = content;
    // Count number of lines
    while ((tmp = strstr(tmp, delim)) != NULL) {
        count++;
        tmp += 2; // Skip the delimiter
    }
    // Allocate array of pointers to char
    char** lines = malloc((count + 1) * sizeof(char*)); // +1 for the last line if no trailing \r\n
    assert(lines);
    int i = 0;
    char* start = content;
    while ((tmp = strstr(start, delim)) != NULL) {
        //int len = tmp - start;
        int len = tmp - start + 2; // Include \r\n in the line length
        lines[i] = malloc((len + 1) * sizeof(char)); // +1 for '\0'
        if (!lines[i]) {
            perror("Failed to allocate memory for a line");
            // Free previously allocated memory
            while (i-- > 0) free(lines[i]);
            free(lines);
            return NULL;
        }
        strncpy(lines[i], start, len);
        lines[i][len] = '\0';
        start = tmp + 2;
        i++;
    }
    // Handle the last line if there is no trailing \r\n
    if (*start) {
        int len = strlen(start);
        lines[i] = malloc((len + 1) * sizeof(char));
        if (!lines[i]) {
            perror("Failed to allocate memory for the last line");
            while (i-- > 0) free(lines[i]);
            free(lines);
            return NULL;
        }
        strcpy(lines[i], start);
        i++;
    }
    *line_count = i; // number of lines actually stored
    return lines;
}

// find th index of boundry
int find_boundary_index(char **lines, int line_count, char *boundary) {
    for (int i = 0; i < line_count; i++) {
        if (strcmp(lines[i], boundary) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to find the start index of the content within the body of an email message.
int find_content_start_index(char **lines, int line_count, int body_start_index, int body_end_index) {
    int content_start_index = body_start_index; // Initialize with the index where the body starts
    int header_index1 = 0, header_index2 = 0; // To keep track of the end of relevant headers

    // Iterate through the lines within the body boundaries
    while (content_start_index < body_end_index) {
        char complete_header[BUFFER_SIZE];  // To store the complete header content

        // Check if the current line starts with "content-type" header
        if (starts_with_header(lines[content_start_index], "content-type") == 1) {
            // Read the complete header content
            int end_header = read_complete_header(lines, line_count, complete_header, content_start_index);
            // Check if the header specifies UTF-8 text/plain content
            if (strstr(complete_header, "text/plain; charset=UTF-8") != NULL) {
                header_index1 = end_header;
            }
        // Check if the current line starts with "content-transfer-encoding" header
        } else if (starts_with_header(lines[content_start_index], "content-transfer-encoding") == 1) {
            // Read the complete header content
            int end_header = read_complete_header(lines, line_count, complete_header, content_start_index);
            // Check for specific encoding types
            if (strstr(complete_header, "quoted-printable") != NULL || strstr(complete_header, "7bit") != NULL || strstr(complete_header, "8bit") != NULL) {
                header_index2 = end_header;
            }
        }
        content_start_index++; // Move to the next line
        // If both headers are found, break the loop
        if (header_index1 != 0 && header_index2 != 0) {
            break;
        }
    }
    // If either header is not found, print error and exit
    if (header_index1 == 0 || header_index2 == 0){
        fprintf(stderr, "Can't find matched content\n");
        exit(3);
    }

    return (header_index1 > header_index2) ? header_index1 : header_index2;
}

// Function to print the content from the specified start index to the end boundary.
void print_content(char **lines, int content_start_index, int body_end_index, char *startBoundary) {
    int print_index = content_start_index + 1;
     // Iterate over lines until the end boundary of the body
    while (print_index < body_end_index) {
        // Check if the current and next line denote the end of the content
        if (strcmp(lines[print_index], "\r\n") == 0 && strcmp(lines[print_index + 1], startBoundary) == 0) {
            break; // Stop printing if end boundary is reached
        } else {
            printf("%s", lines[print_index]); // Print the line
        }
        print_index++; // Move to the next line
    }
}

// free the memory 
void free_lines(char **lines, int line_count) {
    if (lines != NULL) {
        for (int i = 0; i < line_count; i++) {
            free(lines[i]); // Free each line's memory
        }
        free(lines); // Free the array of pointers
    }
}

int read_complete_header(char **lines, int line_count, char *complete_header, int start_index){
    char *line;
    complete_header[0] = '\0';  // Ensure it starts empty
    int i = start_index;
    line = lines[i];
    while (i<line_count){
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            if (len > 1 && line[len - 2] == '\r') {
                line[len - 2] = '\0';  // delete\r
            } else {
                line[len - 1] = '\0';  // delete\n
            }
        }

        // Create a temporary string to handle double quotes
        char temp_line[BUFFER_SIZE];
        int temp_index = 0;

        // Copy and ignore double quotes
        for (size_t j = 0; j < strlen(line); j++) {
            if (line[j] != '"') {
                temp_line[temp_index++] = line[j];
            }
        }
        temp_line[temp_index] = '\0';

        // Check for ";" at the end of the line
        if (temp_line[strlen(temp_line) - 1] == ';') {
            strcat(complete_header, temp_line);  // Adds the current row to the header
            strcat(complete_header, " ");  // Add a space for separation
        } else {
            strcat(complete_header, temp_line);  // Add the last line to the header
            break;  
        }
        i++;
        // Read the next line and check for whitespace at the beginning
        line = lines[i];
        //char *start = line;
        while (*line == ' ' || *line == '\t') {  // Skip leading Spaces or tabs
            line++;
        }

    }
    return ++i;
}