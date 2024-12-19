#include "utils.h"

// Mutex for thread-safe socket creation
pthread_mutex_t sock_mutex = PTHREAD_MUTEX_INITIALIZER;

// Client session structure
typedef struct {
    int sock;
    struct sockaddr_in addr;
    char username[256];
    bool is_guest;
} ClientSession;

// Function declarations
void* handle_client(void* arg);
void handle_auth_request(ClientSession* session);
void handle_lookup_request(ClientSession* session, const char* target);
void handle_push_request(ClientSession* session, const char* filename);
void handle_remove_request(ClientSession* session, const char* filename);
void handle_deploy_request(ClientSession* session);
void cleanup_session(ClientSession* session);

// Helper functions for server connections
int connect_to_server_a();
int connect_to_server_r();
int connect_to_server_d();

int main() {
    int tcp_sock;
    struct sockaddr_in server_addr;
    
    // Create TCP socket for client connections
    tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcp_sock < 0) {
        // free(tcp_sock);
        error("Error creating TCP socket");
    }

    // Enable address reuse
    int reuse = 1;
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        error("Error setting socket option");

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    server_addr.sin_port = htons(TCP_PORT_M);

    // Bind TCP socket
    if (bind(tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
        error("Error binding TCP socket");

    // Listen for client connections
    if (listen(tcp_sock, MAX_CONNECTIONS) < 0) 
        error("Error listening for connections");

    printf("Server M is up and running using UDP on port %d.\n", UDP_PORT_M);

    // Main server loop
    while (1) {
        ClientSession* session = malloc(sizeof(ClientSession));
        if (!session) {
            perror("Failed to allocate session");
            continue;
        }

        socklen_t addr_len = sizeof(session->addr);
        session->sock = accept(tcp_sock, (struct sockaddr*)&session->addr, &addr_len);
        
        if (session->sock < 0) {
            perror("Error accepting connection");
            free(session);
            continue;
        }

        // Create thread to handle client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, session) != 0) {
            perror("Failed to create thread");
            close(session->sock);
            free(session);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(tcp_sock);
    pthread_mutex_destroy(&sock_mutex);
    return 0;
}

int connect_to_server_a() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(UDP_PORT_A);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int connect_to_server_r() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(UDP_PORT_R);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int connect_to_server_d() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(UDP_PORT_D);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

void handle_auth_request(ClientSession* session) {
    char buffer[BUFFER_SIZE];
    char username[256], password[256];
    
    // Read authentication request
    bzero(buffer, BUFFER_SIZE);
    read(session->sock, buffer, BUFFER_SIZE-1);
    
    // Parse auth request
    sscanf(buffer, "AUTH %s %s", username, password);
    strncpy(session->username, username, sizeof(session->username) - 1);
    
    printf("Server M has received username %s and password ****.\n", username);

    // Handle guest login
    if (strcmp(username, "guest") == 0 && strcmp(password, encrypt_password("guest")) == 0) {
        session->is_guest = true;
        write(session->sock, "GUEST_OK", 8);
        return;
    } 

    session->is_guest = false;

    // Connect to Server A only when needed
    pthread_mutex_lock(&sock_mutex);
    int server_a_sock = connect_to_server_a();
    if (server_a_sock < 0) {
        write(session->sock, "SERVER_ERROR", 12);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    // Forward auth request to Server A
    printf("Server M has sent authentication request to Server A\n");
    send(server_a_sock, buffer, strlen(buffer), 0);

    // Get response from Server A
    bzero(buffer, BUFFER_SIZE);
    recv(server_a_sock, buffer, BUFFER_SIZE-1, 0);

    // Close connection to Server A
    close(server_a_sock);
    pthread_mutex_unlock(&sock_mutex);

    printf("The main server has received the response from server A using UDP over %d\n", 
           UDP_PORT_M);

    // Forward response to client
    write(session->sock, buffer, strlen(buffer));
    
    printf("The main server has sent the response from server A to client using TCP over port %d.\n",
           TCP_PORT_M);
}

void handle_lookup_request(ClientSession* session, const char* target) {
    char buffer[BUFFER_SIZE];
    const char* lookup_user = target[0] ? target : session->username;

    if (strcmp(session->username, "guest") == 0) {
        printf("The main server has received a lookup request from Guest to lookup %s’s repository \
            using TCP over port %d.", lookup_user, TCP_PORT_M);
    } else {
        printf("The main server has received a lookup request from %s to lookup %s’s repository \
            using TCP over port %d.", session->username, lookup_user, TCP_PORT_M);
    }
    
    // Connect to Server R only when needed
    pthread_mutex_lock(&sock_mutex);
    int server_r_sock = connect_to_server_r();
    if (server_r_sock < 0) {
        write(session->sock, "SERVER_ERROR", 12);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    // Format and send lookup request
    sprintf(buffer, "LOOKUP %s", lookup_user);
    printf("The main server has sent the lookup request to server R.\n");
    send(server_r_sock, buffer, strlen(buffer), 0);

    // Get response from Server R
    bzero(buffer, BUFFER_SIZE);
    recv(server_r_sock, buffer, BUFFER_SIZE-1, 0);

    // Close connection to Server R
    close(server_r_sock);
    pthread_mutex_unlock(&sock_mutex);

    printf("The main server has received the response from server R using UDP over %d\n",
           UDP_PORT_M);

    // Forward response to client
    write(session->sock, buffer, strlen(buffer));
    printf("The main server has sent the response to the client.\n");
}

void handle_push_request(ClientSession* session, const char* filename) {
    char buffer[BUFFER_SIZE];
    
    // Connect to Server R only when needed
    pthread_mutex_lock(&sock_mutex);
    int server_r_sock = connect_to_server_r();
    if (server_r_sock < 0) {
        write(session->sock, "SERVER_ERROR", 12);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    // Format and send push request
    // sprintf(buffer, "PUSH %s %s", session->username, filename);
    sprintf(buffer, "PUSH %s %s", session->username, filename);
    printf("The main server has received a push request from %s, using TCP over port %d.\n",
           session->username, TCP_PORT_M);
    
    send(server_r_sock, buffer, strlen(buffer), 0);
    printf("The main server has sent the push request to server R.\n");
    
    // Get initial response
    bzero(buffer, BUFFER_SIZE);
    recv(server_r_sock, buffer, BUFFER_SIZE-1, 0);

    if (strncmp(buffer, "CONFIRM", 7) == 0) {
        printf("The main server has received the response from server R using UDP over %d, \
                asking for overwrite confirmation\n", UDP_PORT_M);

        // Handle overwrite confirmation
        write(session->sock, buffer, strlen(buffer));
        printf("The main server has sent the overwrite confirmation request to the client.\n");
        
        // Get client response
        bzero(buffer, BUFFER_SIZE);
        read(session->sock, buffer, BUFFER_SIZE-1);
        printf("The main server has received the overwrite confirmation response from %s using TCP over port %d\n",
               session->username, TCP_PORT_M);

        // Forward response to Server R
        send(server_r_sock, buffer, strlen(buffer), 0);
        printf("The main server has sent the overwrite confirmation response to server R.\n");
        
        // Get final response
        bzero(buffer, BUFFER_SIZE);
        recv(server_r_sock, buffer, BUFFER_SIZE-1, 0);
        printf("The main server has received the response from server R using UDP over %d\n", UDP_PORT_M);
    } else {
        printf("The main server has received the response from server R using UDP over %d\n", UDP_PORT_M);
    }

    // Close connection to Server R
    close(server_r_sock);
    pthread_mutex_unlock(&sock_mutex);

    // Send final response to client
    write(session->sock, buffer, strlen(buffer));
    printf("The main server has sent the response to the client.\n");
}

void handle_deploy_request(ClientSession* session) {
    char buffer[BUFFER_SIZE];
    char* file_list = NULL;
    char* deploy_msg = NULL;
    
    printf("The main server has received a deploy request from member %s TCP over port %d.\n",
           session->username, TCP_PORT_M);

    // First connect to Server R to get file list
    pthread_mutex_lock(&sock_mutex);
    int server_r_sock = connect_to_server_r();
    if (server_r_sock < 0) {
        write(session->sock, "SERVER_ERROR", 12);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    // Get file list from Server R
    sprintf(buffer, "DEPLOY %s", session->username);
    printf("The main server has sent the lookup request to server R.\n");
    send(server_r_sock, buffer, strlen(buffer), 0);
    
    bzero(buffer, BUFFER_SIZE);
    recv(server_r_sock, buffer, BUFFER_SIZE-1, 0);
    printf("The main server received the lookup response from server R.\n");
    
    
    // Close Server R connection
    close(server_r_sock);

    // Check if repository is empty or user not found
    if (strncmp(buffer, "EMPTY", 5) == 0) {
        write(session->sock, "EMPTY", 5);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    if (strncmp(buffer, "NOTFOUND", 8) == 0) {
        write(session->sock, "NOTFOUND", 8);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    // Allocate memory for file_list and copy data
    file_list = malloc(strlen(buffer) + 1);
    if (!file_list) {
        write(session->sock, "FAIL", 4);
        pthread_mutex_unlock(&sock_mutex);
        error("Memory allocation failed for file_list");
    }
    
    // Store file list for response to client
    strcpy(file_list, buffer);

    // Connect to Server D to deploy files
    int server_d_sock = connect_to_server_d();
    if (server_d_sock < 0) {
        free(file_list);
        pthread_mutex_unlock(&sock_mutex);
        error("Failed to connect to Server D");
    }

    // 计算需要的总缓冲区大小
    size_t required_size = strlen("DEPLOY ") + strlen(session->username) + strlen(file_list) + 3; //+1 是 '\0' 结尾
    
    // 动态分配足够大的内存
    deploy_msg = malloc(required_size);
    if (!deploy_msg) {
        free(file_list);
        close(server_d_sock);
        pthread_mutex_unlock(&sock_mutex);
        error("Memory allocation failed for deploy message");
    }

    // Send deploy request to Server D
    snprintf(deploy_msg, required_size, "DEPLOY %s %s", session->username, file_list);
    printf("The main server has sent the deploy request to server D.\n");
    
    send(server_d_sock, deploy_msg, strlen(deploy_msg), 0);
    free(deploy_msg);  // Free deploy message as it's no longer needed

    // Get deployment confirmation
    bzero(buffer, BUFFER_SIZE);
    recv(server_d_sock, buffer, BUFFER_SIZE-1, 0);
    
    // Close Server D connection
    close(server_d_sock);
    pthread_mutex_unlock(&sock_mutex);

    // Check Server D's response
    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("The user %s's repository has been deployed at server D.\n", session->username);
        // Send the file list to client
        write(session->sock, file_list, strlen(file_list));
    } else {
        printf("Error deploying %s's repository at server D.\n", session->username);
        write(session->sock, "FAIL", 4);
    }
}

void handle_remove_request(ClientSession* session, const char* filename) {
    char buffer[BUFFER_SIZE];
    
    // Connect to Server R only when needed
    pthread_mutex_lock(&sock_mutex);
    int server_r_sock = connect_to_server_r();
    if (server_r_sock < 0) {
        write(session->sock, "SERVER_ERROR", 12);
        pthread_mutex_unlock(&sock_mutex);
        return;
    }

    // Format and send remove request
    sprintf(buffer, "REMOVE %s %s", session->username, filename);
    printf("The main server has received a remove request from member %s TCP over port %d.\n",
           session->username, TCP_PORT_M);
    send(server_r_sock, buffer, strlen(buffer), 0);

    // Get response
    bzero(buffer, BUFFER_SIZE);
    recv(server_r_sock, buffer, BUFFER_SIZE-1, 0);

    // Close connection to Server R
    close(server_r_sock);
    pthread_mutex_unlock(&sock_mutex);

    printf("The main server has received confirmation of the remove request done by the server R\n");

    // Forward response to client
    write(session->sock, buffer, strlen(buffer));
}

void* handle_client(void* arg) {
    ClientSession* session = (ClientSession*)arg;
    char buffer[BUFFER_SIZE];
    
    // Handle authentication first
    handle_auth_request(session);
    
    // Main command loop
    while (1) {
        bzero(buffer, BUFFER_SIZE);
        ssize_t n = read(session->sock, buffer, BUFFER_SIZE-1);
        
        if (n <= 0) { // Client disconnected or error
            break;
        }

        char command[32], username[MAX_USERNAME], filename[MAX_FILENAME];
        command[0] = username[0] = filename[0] = '\0';
        sscanf(buffer, "%s %s %s", command, username, filename);

        Command cmd = parse_command(command);

        // Handle commands based on type
        switch (cmd.type) {
            case CMD_LOOKUP:
                handle_lookup_request(session, username);
                break;
            case CMD_PUSH:
                if (!session->is_guest)
                    handle_push_request(session, filename);
                else
                    write(session->sock, "Guests can only use the lookup command", 37);
                break;
            case CMD_REMOVE:
                if (!session->is_guest)
                    handle_remove_request(session, filename);
                else
                    write(session->sock, "Guests can only use the lookup command", 37);
                break;
            case CMD_DEPLOY:
                if (!session->is_guest)
                    handle_deploy_request(session);
                else
                    write(session->sock, "Guests can only use the lookup command", 37);
                break;
            default:
                write(session->sock, "Invalid command", 14);
                break;
        }
    }

    cleanup_session(session);
    return NULL;
}

void cleanup_session(ClientSession* session) {
    if (session) {
        if (session->sock >= 0) {
            close(session->sock);
        }
        free(session);
    }
}
