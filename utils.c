#include "utils.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <regex.h>

#include <sys/socket.h>

// Handle possible multi-line topics
void unfold_header(char *header) {
    char *src = header, *dst = header;
    while (*src) {
        // Check for folded line: CRLF followed by a white space (space or tab)
        if (*src == '\r' && *(src + 1) == '\n' && (*(src + 2) == ' ' || *(src + 2) == '\t')) {
            src += 2; // Skip the CRLF
            if (*(src) == '\t') {
                *dst++ = '\t'; // If it's a tab, keep the tab
            } else {
                *dst++ = ' '; // Insert a single space to maintain separation if it's a space
            }
            src++; // Move past the space/tab that has been handled
            while (*src == ' ' || *src == '\t') {
                src++; // Skip any additional white space following the initial space/tab after CRLF
            }
        } else if (*src == '\r') {
            src++; // Skip the carriage return if it's not part of a CRLF
        } else {
            *dst++ = *src++; // Copy other characters
        }
    }
    *dst = '\0'; // Null-terminate the unfolded header

    // Remove trailing newline if present
    if (dst != header && *(dst - 1) == '\n') {
        *(dst - 1) = '\0';
    }
}


void to_lower_string(const char *src, char *dest) {
    while (*src) {
        *dest++ = tolower((unsigned char)*src++);  // Convert and copy
    }
    *dest = '\0';  // Null-terminate the destination string
}

int validate_username(const char *username) {
    while (*username) {
        if (!isalnum(*username) && !strchr("@_-.", *username)) {
            return 0;
        }
        username++;
    }
    return 1;
}

int validate_password(const char *password) {
    while (*password) {
        if (!isalnum(*password) && !strchr("!@#$%^&*()_+=-", *password)) {
            return 0;
        }
        password++;
    }
    return 1;
}

int validate_folder_name(const char *folder_name) {
    if (folder_name == NULL){
        return 1;
    }
    while (*folder_name) {
        if (!isalnum(*folder_name) && *folder_name != '_' && *folder_name != '-' && *folder_name != ' ') {
            return 0;
        }
        folder_name++;
    }
    return 1;
}

char *sanitize_input(const char *input) {
    size_t len = strlen(input);
    char *sanitized = malloc(len * 2 + 1); 
    assert(sanitized);
    char *p = sanitized;

    while (*input) {
        if (*input == '\\' || *input == '"') {
            *p++ = '\\';
        }
        *p++ = *input++;
    }
    *p = '\0';
    
    return sanitized;
}

// // Validate folder name: assume folder names are alphanumeric, can include underscores, dashes, and slashes.
// int validate_folder_name(const char *folder_name) {
//     regex_t regex;
//     int result;
//     // Compile regex: ^[a-zA-Z0-9_/-]+$
//     if (regcomp(&regex, "^[a-zA-Z0-9 _/-]+$", REG_EXTENDED)) {
//         fprintf(stderr, "Could not compile regex\n");
//         exit(EXIT_FAILURE);
//     }
//     result = regexec(&regex, folder_name, 0, NULL, 0);
//     regfree(&regex);
//     return !result;  // Return 1 if the pattern matches, 0 otherwise
// }

// Validate message number: assume it's a positive integer
int validate_message_number(const char *messageNum) {
    regex_t regex;
    int result;
    // Compile regex: ^[0-9]+$
    if (regcomp(&regex, "^[0-9]+$", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
        exit(EXIT_FAILURE);
    }
    result = regexec(&regex, messageNum, 0, NULL, 0);
    regfree(&regex);
    return !result;  // Return 1 if the pattern matches, 0 otherwise
}
