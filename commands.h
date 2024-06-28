
#ifndef COMMANDS_H
#define COMMANDS_H

#define BUFFER_SIZE 1024
#define BUFFER_NOTENOUGH 10
#define SUBJECT_STRLEN 9
#define SUBJECT_END2 2
#define END_OF_STRING 1

// // log into the server
void login_to_imap_server(int sockfd, const char* username, const char* password);
// select folder which want to read from
void select_folder(int sockfd, const char* folder);
//  fetch the email, If the message sequence number was not specified on the command line, then fetch the last 
// added message in the folder
void retrieve_message(int sockfd, char *messageNum);
// change the structure of retrieve_message
void parse_message(int sockfd, char *messageNum);
// checks the MIME and content type of a message by its number and handles the message accordingly.
void find_mime(int sockfd, char *messageNum);
// print to stdout the subject lines of all the emails in the specified folder, sorted by message sequence number
void list_subjects(int sockfd, const char* folder);

#endif