#include "utils.h"

void handle_guest();
void handle_member(int sock, char *username, char *password);
void handle_lookup(int sockfd, const char *username, const char *target_user, bool is_guest);
void handle_push(int sockfd, const char *username, const char *filename);
void handle_deploy(int sockfd, const char *username);
void handle_remove(int sockfd, const char *username, const char *filename);

static int _port = 0;

int main(int argc, char *argv[]) {
    // Validate input arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: ./client <username> <password>\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    bool is_guest = false;

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) error("Error opening socket");

    // Bind client socket
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = 0;  // Let system assign port
    client_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        close(sockfd);
        error("Error binding client socket");
    }

    // Get assigned port
    if (getsockname(sockfd, (struct sockaddr *)&client_addr, &addr_len) < 0) {
        close(sockfd);
        error("Error getting socket name");
    }

    _port = ntohs(client_addr.sin_port);

    // Initialize server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT_M);
    serv_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        error("Error connecting to ServerM");
    }
    printf("The client is up and running.\n");

    // Send authentication request
    // char *username = argv[1];
    char *encrypted_password = encrypt_password(argv[2]);
    sprintf(buffer, "AUTH %s %s", argv[1], encrypted_password);
    write(sockfd, buffer, strlen(buffer));

    // Receive authentication response
    bzero(buffer, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE-1);

    if (strcmp(buffer, "GUEST_OK") == 0) {
        printf("You have been granted guest access.\n");
        is_guest = true;
    }
    else if (strcmp(buffer, "MEMBER_OK") == 0) {
        printf("You have been granted member access\n");
    }
    else {
        printf("The credentials are incorrect. Please try again.\n");
        close(sockfd);
        return 1;
    }

    // Main command loop
    while (1) {
        if (is_guest) {
            printf("Please enter the command: <lookup <username>>\n");
        } else {
            printf("Please enter the command:\n"
                   "<lookup <username>>\n"
                   "<push <filename>>\n"
                   "<remove <filename>>\n"
                   "<deploy>\n"
                   "<log>\n");
        }

        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE-1, stdin);
        
        Command cmd = parse_command(buffer);
        
        if (is_guest && cmd.type != CMD_LOOKUP) {
            printf("Guests can only use the lookup command\n");
            continue;
        }

        switch (cmd.type) {
            case CMD_LOOKUP:
                handle_lookup(sockfd, argv[1], cmd.arg, is_guest);
                break;
            case CMD_PUSH:
                if (!is_guest) handle_push(sockfd, argv[1], cmd.arg);
                break;
            case CMD_REMOVE:
                if (!is_guest) handle_remove(sockfd, argv[1], cmd.arg);
                break;
            case CMD_DEPLOY:
                if (!is_guest) handle_deploy(sockfd, argv[1]);
                break;
            default:
                printf("Invalid command. Please try again.\n----Start a new request----\n");
                break;
        }
    }

    close(sockfd);
    return 0;
}

// Operations handling
// Look up
void handle_lookup(int sockfd, const char *username, const char *target_user, bool is_guest) {
    char buffer[BUFFER_SIZE];
    
    // For guest users, must specify target username
    // if (strcmp(username, "guest") == 0 && !target_user[0]) {
    if (is_guest && !target_user[0]) {
        printf("Error: Username is required. Please specify a username to lookup.\n");
        printf("----Start a new request----\n");
        return;
    }

    // For member users, if no target specified, use their own username
    const char *lookup_user;
    // if (!target_user[0] && strcmp(username, "guest") != 0) {
    if (!target_user[0] && !is_guest) {
        lookup_user = username;
        printf("Username is not specified. Will lookup %s.\n", username);
    } else {
        lookup_user = target_user;
    }
    
    sprintf(buffer, "LOOKUP %s", lookup_user);
    write(sockfd, buffer, strlen(buffer));
    
    printf("%s sent a lookup request to the main server.\n", 
           is_guest ? "Guest" : username);
        //    strcmp(username, "guest") == 0 ? "Guest" : username);

    // Receive response
    bzero(buffer, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE-1);
    
    printf("The client received the response from the main server using TCP over port %d.\n", 
           ntohs(_port));
           // ntohs(((struct sockaddr_in*)&sockfd)->sin_port));

    if (strncmp(buffer, "EMPTY", 5) == 0) {
        printf("Empty repository.\n");
    } else if (strncmp(buffer, "NOTFOUND", 8) == 0) {
        printf("%s does not exist. Please try again.\n", lookup_user);
    } else {
        printf("%s\n", buffer); // Print file list
    }
    printf("----Start a new request----\n");
}

// Push
void handle_push(int sockfd, const char *username, const char *filepath) {
    char buffer[BUFFER_SIZE];
    char filename[MAX_FILENAME];

    if (!filepath[0]) {
        printf("Error: Filename is required. Please specify a filename to push.\n");
        printf("----Start a new request----\n");
        return;
    }

    // Extract filename from filepath
    const char *last_slash = strrchr(filepath, '/');
    if (last_slash) {
        // If path contains slashes, take everything after the last slash
        strncpy(filename, last_slash + 1, MAX_FILENAME - 1);
    } else {
        // If no slashes, use the entire filepath as filename
        strncpy(filename, filepath, MAX_FILENAME - 1);
    }
    filename[MAX_FILENAME - 1] = '\0';  // Ensure null termination

    // Check if filename is empty after extraction
    if (filename[0] == '\0') {
        printf("Error: Invalid filename\n");
        printf("----Start a new request----\n");
        return;
    }

    // Check if file exists and is readable
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        printf("Error: Invalid file: %s\n", filename);
        printf("----Start a new request----\n");
        return;
    }
    fclose(file);  // Close the file as we only needed to check access

    // Send push request
    sprintf(buffer, "PUSH %s %s", username, filename);
    write(sockfd, buffer, strlen(buffer));
    
    // Get initial response
    bzero(buffer, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE-1);

    if (strncmp(buffer, "CONFIRM", 7) == 0) {
        printf("%s exists in %s's repository, do you want to overwrite (Y/N)?\n", 
               filename, username);
        
        char response[2];
        scanf("%1s", response);
        getchar(); // Clear newline

        // Send confirmation response
        write(sockfd, response, 1);
        
        // Get final response
        bzero(buffer, BUFFER_SIZE);
        read(sockfd, buffer, BUFFER_SIZE-1);
    }

    // Handle server response
    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("%s pushed successfully\n", filename);
    } else {
        printf("%s was not pushed successfully\n", filename);
    }

    // printf("----Start a new request----\n");
}

// Deploy
void handle_deploy(int sockfd, const char *username) {
    char buffer[BUFFER_SIZE];
    
    sprintf(buffer, "DEPLOY %s", username);
    write(sockfd, buffer, strlen(buffer));
    
    printf("%s sent a deploy request to the main server.\n", username);

    bzero(buffer, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE-1);
    
    printf("The client received the response from the main server using TCP over port %d.\n",
           ntohs(_port));
           
    if (strncmp(buffer, "EMPTY", 5) == 0) {
        printf("Empty repository.\n");
    } else if (strncmp(buffer, "NOTFOUND", 8) == 0) {
        printf("%s does not exist. Please try again.\n", username);
    } else if (strncmp(buffer, "FAIL", 4) == 0) {
        printf("Error: Deployment failed.\n");
    } else {
        printf("The following files in his/her repository have been deployed.\n");
        printf("%s", buffer); // Print file list
    }
    
    printf("----Start a new request----\n");
}

// Remove
void handle_remove(int sockfd, const char *username, const char *filename) {
    char buffer[BUFFER_SIZE];
    
    if (!filename[0]) {
        printf("Error: Filename is required. Please specify a filename to remove.\n");
        printf("----Start a new request----\n");
        return;
    }

    sprintf(buffer, "REMOVE %s %s", username, filename);
    write(sockfd, buffer, strlen(buffer));
    
    printf("%s sent a remove request to the main server.\n", username);

    bzero(buffer, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE-1);
    
    printf("%s\n", buffer);
}


void handle_guest() {
    printf("The client is up and running.\n");
    printf("You have been granted guest access.\n");
    printf("Please enter the command: <lookup <username>>\n");
    // Guests can only perform lookup operations.
}

void handle_member(int sock, char *username, char *password) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Send username and password to the server
    send(sock, username, strlen(username), 0);
    send(sock, " ", 1, 0); // Separator

    char *encrypted_password = encrypt_password(password);
    send(sock, password, strlen(encrypted_password), 0);

    // Receive the authentication response
    bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("Failed to receive data from server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';

    // Print server response
    if (strstr(buffer, "success")) {
        printf("You have been granted member access.\n");
        printf("Please enter the command:\n");
        printf("<lookup <username>>\n<push <filename>>\n<remove <filename>>\n<deploy>\n<log>\n");
    } else {
        printf("The credentials are incorrect. Please try again.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
}


