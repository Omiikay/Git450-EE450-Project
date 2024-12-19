#include "utils.h"

void handle_request(int sock, struct sockaddr_in* client_addr);

int main() {
    int sock;
    struct sockaddr_in server_addr;
    
    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) error("Error creating socket");

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    server_addr.sin_port = htons(UDP_PORT_D);

    // Bind socket
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        error("Error binding socket");
    }

    printf("Server D is up and running using UDP on port %d.\n", UDP_PORT_D);

    struct sockaddr_in client_addr;
    while (1) {
        handle_request(sock, &client_addr);
    }

    close(sock);
    return 0;
}

void handle_request(int sock, struct sockaddr_in* client_addr) {
    char buffer[BUFFER_SIZE];
    char username[MAX_USERNAME], file_list[BUFFER_SIZE];
    socklen_t addr_len = sizeof(*client_addr);

    // Receive request
    bzero(buffer, BUFFER_SIZE);
    recvfrom(sock, buffer, BUFFER_SIZE-1, 0,
             (struct sockaddr*)client_addr, &addr_len);

    printf("Server D has received a deploy request from the main server.\n");

    // Parse deploy request (format: "DEPLOY username filelist")
    sscanf(buffer, "DEPLOY %s", username);
    char* file_start = strstr(buffer, username) + strlen(username) + 1;
    if (file_start) {
        strncpy(file_list, file_start, BUFFER_SIZE - 1);
        file_list[BUFFER_SIZE - 1] = '\0';
    }

    // Open or create deployed.txt
    FILE* file = fopen("deployed.txt", "a+");
    if (!file) {
        perror("Error opening deployed.txt");
        sendto(sock, "FAIL", 4, 0, 
               (struct sockaddr*)client_addr, addr_len);
        return;
    }

    // If file is empty, write header
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) {
        fprintf(file, "Username Filename\n");
    }

    // Parse and write each filename
    char* file_list_copy = strdup(file_list);
    char* filename = strtok(file_list_copy, "\n");
    while (filename) {
        fprintf(file, "%s %s\n", username, filename);
        filename = strtok(NULL, "\n");
    }

    free(file_list_copy);
    fclose(file);

    printf("Server D has deployed the user %s's repository.\n", username);

    // Send success response
    sendto(sock, "SUCCESS", 7, 0, (struct sockaddr*)client_addr, addr_len);
}