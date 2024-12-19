#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define SUFFIX 446
#define UDP_PORT_A 21000 + SUFFIX
#define UDP_PORT_R 22000 + SUFFIX
#define UDP_PORT_D 23000 + SUFFIX
#define UDP_PORT_M 24000 + SUFFIX
#define TCP_PORT_M 25000 + SUFFIX
 
#define MAX_FILENAME 256
#define MAX_USERNAME 256
#define MAX_PASSWORD 256
#define MAX_CONNECTIONS 128
#define BUFFER_SIZE 1024
#define LOCALHOST "127.0.0.1"

#define SUCCESS 0
#define FAILURE -1
#define NOTFOUND -2

#define MATCH 0
#define MISMATCH -1

typedef enum {
    CMD_LOOKUP,
    CMD_PUSH,
    CMD_REMOVE,
    CMD_DEPLOY,
    CMD_INVALID
} CommandType;

typedef struct {
    CommandType type;
    char arg[MAX_FILENAME];
} Command;

void error(const char *msg);
char *encrypt_password(char *password);
Command parse_command(char *input);
#endif /* UTILS_H */