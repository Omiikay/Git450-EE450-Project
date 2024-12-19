#include "utils.h"
#include <ctype.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Encrypt a given string using the specified scheme
char *encrypt_password(char *password) {
    int i, length = strlen(password);
    char *encrypted = (char *)malloc(length + 1); // Allocate memory for encrypted string

    if (!encrypted) { // Ensure memory allocation succeeded
        perror("Memory allocation failed");
        return NULL;
    }

    for (i = 0; i < length; i++) {
        char ch = password[i];

        // Alphabetic character encryption (case-sensitive)
        if (isalpha(ch)) {
            if (islower(ch)) {
                encrypted[i] = ((ch - 'a' + 3) % 26) + 'a'; // Wrap within 'a'-'z'
            } else if (isupper(ch)) {
                encrypted[i] = ((ch - 'A' + 3) % 26) + 'A'; // Wrap within 'A'-'Z'
            }
        }
        // Numeric character encryption (cyclic 0-9)
        else if (isdigit(ch)) {
            encrypted[i] = ((ch - '0' + 3) % 10) + '0'; // Wrap within '0'-'9'
        }
        // Special characters remain unchanged
        else {
            encrypted[i] = ch;
        }
    }

    encrypted[length] = '\0'; // Null-terminate the encrypted string
    return encrypted;
}


Command parse_command(char *input) {
    Command cmd = {CMD_INVALID, ""};
    char *token = strtok(input, " \n");
    
    if (!token) return cmd;

    if (strcmp(token, "lookup") == 0 || strcmp(token, "LOOKUP") == 0) {
        cmd.type = CMD_LOOKUP;
        token = strtok(NULL, " \n");
        if (token) strncpy(cmd.arg, token, MAX_FILENAME-1);
    }
    else if (strcmp(token, "push") == 0 || strcmp(token, "PUSH") == 0) {
        cmd.type = CMD_PUSH;
        token = strtok(NULL, " \n");
        if (token) strncpy(cmd.arg, token, MAX_FILENAME-1);
    }
    else if (strcmp(token, "remove") == 0 || strcmp(token, "REMOVE") == 0) {
        cmd.type = CMD_REMOVE;
        token = strtok(NULL, " \n");
        if (token) strncpy(cmd.arg, token, MAX_FILENAME-1);
    }
    else if (strcmp(token, "deploy") == 0 || strcmp(token, "DEPLOY") == 0) {
        cmd.type = CMD_DEPLOY;
    }

    return cmd;
}
