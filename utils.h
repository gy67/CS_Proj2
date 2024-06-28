#ifndef UTILS_H
#define UTILS_H

// Handle possible multi-line topics
void unfold_header(char *header);
void to_lower_string(const char *src, char *dest);
// Validate username: assume usernames are alphanumeric and can include underscores and dashes, but must start with a letter.
int validate_username(const char *username);
int validate_password(const char *password);
// Validate folder name: assume folder names are alphanumeric, can include underscores, dashes, and slashes.
int validate_folder_name(const char *folder_name);
// Validate message number: assume it's a positive integer
int validate_message_number(const char *messageNum);
// int validate_input(char *input);
int validate_input(char *input, char *compare_object);
char *sanitize_input(const char *input);

#endif