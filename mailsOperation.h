
#ifndef MAIL_OPERATIONS_H
#define MAIL_OPERATIONS_H

#define BUFFER_SIZE 1024
#define DEFAULT_STR " "
#define DEFAULT_VERSION "1.0"
#define DEFAULT_MEDIA_TYPE "multipart/alternative"

// Read the response from the mail server to determine the size of the message
int fetch_mail_size(int sockfd);
// Read the message content according to the size provided
void fetch_mail_content(int sockfd, char *mail_content, int mail_size);
// Parse and expand the content
void parse_content(char *message, char *content);
// Print the content according to the header information type
void print_parse_info(char *message, char* headers);
// The content after extracting the header information
char *extract_header_content(char *content, char *headers);
// Sends a request to fetch specific header field values of a message and stores the result in 'output'.
void checkHeaderField(int sockfd, char *messageNum, char *field, char *output);
void parseContentType(char *content_type, char *mediaType, char *boundary);
// read a line one time
void read_one_line(int sockfd, char *response_line);
// split the content into lines 
char** split_content_into_lines(char* content, int* line_count);
// find th index of boundry
int find_boundary_index(char **lines, int line_count, char *boundary);
// Function to find the start index of the content within the body of an email message.
int find_content_start_index(char **lines, int line_count, int body_start_index, int body_end_index);
// Function to print the content from the specified start index to the end boundary.
void print_content(char **lines, int content_start_index, int body_end_index, char *startBoundary);
// free the memory 
void free_lines(char **lines, int line_count);
// Helper function to check if a line starts with the specified header
int starts_with_header(char *line, char *header_name);
int read_complete_header(char **lines, int line_count, char *complete_header, int start_index);
#endif