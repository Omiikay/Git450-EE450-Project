#include "utils.h"

// Struct to save user Repository
typedef struct {
    char username[MAX_USERNAME];
    char filename[MAX_FILENAME];
} RepositoryEntry;

RepositoryEntry* repository = NULL;
int entry_count = 0;

// funcs
void load_repository();
void save_repository();
void handle_request(int sock, struct sockaddr_in* client_addr);
void handle_lookup(int sock, struct sockaddr_in* client_addr, const char* username);
void handle_push(int sock, struct sockaddr_in* client_addr, const char* username, const char* filename);
void handle_remove(int sock, struct sockaddr_in* client_addr, const char* username, const char* filename);
void cleanup();

int main() {
    int sock;
    struct sockaddr_in server_addr;

    // 创建UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) error("Error creating socket");

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    server_addr.sin_port = htons(UDP_PORT_R);

    // 绑定socket
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        error("Error binding socket");
    }

    printf("Server R is up and running using UDP on port %d.\n", UDP_PORT_R);

    // 加载仓库数据
    load_repository();

    // 主循环处理请求
    struct sockaddr_in client_addr;
    while (1) {
        handle_request(sock, &client_addr);
    }

    cleanup();
    close(sock);
    return 0;
}

void load_repository() {
    FILE* file = fopen("../data/filenames.txt", "r");
    if (!file) {
        repository = NULL;
        entry_count = 0;
        return;  // empty repository if no matched file
    }

    char line[BUFFER_SIZE];
    size_t capacity = 10;
    entry_count = 0;

    // Skip header line
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        perror("Empty filenames.txt found. Starting with empty repository.\n");
        return;
    }

    // init memory allocate
    repository = malloc(sizeof(RepositoryEntry) * capacity);
    if (!repository) {
        fclose(file);
        error("Memory allocation failed");
    }

    while (fgets(line, sizeof(line), file)) {
        // allocate more memory if needed
        if (entry_count >= capacity) {
            capacity *= 2;
            RepositoryEntry* temp = realloc(repository, sizeof(RepositoryEntry) * capacity);
            if (!temp) {
                free(repository);
                fclose(file);
                error("Memory reallocation failed");
            }
            repository = temp;
        }

        // remove new line symbol
        line[strcspn(line, "\n")] = 0;
        
        char* username = strtok(line, " ");
        char* filename = strtok(NULL, " ");
        
        if (username && filename) {
            strncpy(repository[entry_count].username, username, MAX_USERNAME - 1);
            strncpy(repository[entry_count].filename, filename, MAX_FILENAME - 1);
            repository[entry_count].username[MAX_USERNAME - 1] = '\0';
            repository[entry_count].filename[MAX_FILENAME - 1] = '\0';
            entry_count++;
        }
    }

    fclose(file);
}

void save_repository() {
    FILE* file = fopen("filenames.txt", "w");
    if (!file) error("Error opening filenames.txt for writing");

    // 写入标题行
    fprintf(file, "Username Filename\n");

    // 写入所有条目
    for (int i = 0; i < entry_count; i++) {
        fprintf(file, "%s %s\n", repository[i].username, repository[i].filename);
    }

    fclose(file);
}

void handle_request(int sock, struct sockaddr_in* client_addr) {
    char buffer[BUFFER_SIZE];
    char command[32], username[MAX_USERNAME], filename[MAX_FILENAME];
    socklen_t addr_len = sizeof(*client_addr);

    // 接收请求
    bzero(buffer, BUFFER_SIZE);
    recvfrom(sock, buffer, BUFFER_SIZE-1, 0,
             (struct sockaddr*)client_addr, &addr_len);

    // 解析命令
    command[0] = username[0] = filename[0] = '\0';
    sscanf(buffer, "%s %s %s", command, username, filename);

    if (strcmp(command, "LOOKUP") == 0) {
        printf("Server R has received a lookup request from the main server.\n");
        handle_lookup(sock, client_addr, username);
        printf("Server R has finished sending the response to the main server.\n");
    } else if (strcmp(command, "PUSH") == 0) {
        printf("Server R has received a push request from the main server.\n");
        handle_push(sock, client_addr, username, filename);
    } else if (strcmp(command, "REMOVE") == 0) {
        printf("Server R has received a remove request from the main server.\n");
        handle_remove(sock, client_addr, username, filename);
    } else if (strcmp(command, "DEPLOY") == 0) {
        printf("Server R has received a deploy request from the main server.\n");
        handle_lookup(sock, client_addr, username); // Reuse lookup functionality
        printf("Server R has finished sending the response to the main server.\n");
    }
}

void handle_lookup(int sock, struct sockaddr_in* client_addr, const char* username) {
    char buffer[BUFFER_SIZE] = {0};
    char* pos = buffer;
    bool found = false;
    socklen_t addr_len = sizeof(*client_addr);

    // 查找用户的所有文件
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(repository[i].username, username) == 0) {
            found = true;
            pos += sprintf(pos, "%s\n", repository[i].filename);
        }
    }

    // 发送响应
    if (found && pos != buffer) {
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)client_addr, addr_len);
    } else {
        if (!found) {
            sendto(sock, "NOTFOUND", 8, 0, (struct sockaddr*)client_addr, addr_len);
        } else {
            sendto(sock, "EMPTY", 5, 0, (struct sockaddr*)client_addr, addr_len);
        } 
    }
}

void handle_push(int sock, struct sockaddr_in* client_addr, const char* username, const char* filename) {
    socklen_t addr_len = sizeof(*client_addr);
    bool exists = false;

    // 检查文件是否已存在
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(repository[i].username, username) == 0 &&
            strcmp(repository[i].filename, filename) == 0) {
            exists = true;
            break;
        }
    }

    if (exists) {
        printf("%s exists in %s's repository; requesting overwrite confirmation.\n", 
               filename, username);
        sendto(sock, "CONFIRM", 7, 0, (struct sockaddr*)client_addr, addr_len);

        // 等待确认响应
        char response[2];
        recvfrom(sock, response, 1, 0, (struct sockaddr*)client_addr, &addr_len);
        response[1] = '\0';

        if (response[0] == 'Y' || response[0] == 'y') {
            printf("User requested overwrite; overwrite successful.\n");
            sendto(sock, "SUCCESS", 7, 0, 
                   (struct sockaddr*)client_addr, addr_len);
        } else {
            printf("Overwrite denied\n");
            sendto(sock, "FAIL", 4, 0, 
                   (struct sockaddr*)client_addr, addr_len);
        }
    } else {
        // 添加新文件
        size_t new_size = sizeof(RepositoryEntry) * (entry_count + 1);
        RepositoryEntry* temp = realloc(repository, new_size);
        if (!temp) {
            perror("Server error when adding files.");
            sendto(sock, "FAIL", 4, 0, 
                   (struct sockaddr*)client_addr, addr_len);
            return;
        }
        repository = temp;

        strncpy(repository[entry_count].username, username, 
                sizeof(repository[entry_count].username) - 1);
        strncpy(repository[entry_count].filename, filename, 
                sizeof(repository[entry_count].filename) - 1);
        entry_count++;

        printf("%s uploaded successfully.\n", filename);
        sendto(sock, "SUCCESS", 7, 0, 
               (struct sockaddr*)client_addr, addr_len);
        save_repository();
    }
}

void handle_remove(int sock, struct sockaddr_in* client_addr, const char* username, const char* filename) {
    socklen_t addr_len = sizeof(*client_addr);
    bool found = false;
    int remove_idx = -1;

    // 查找要删除的文件
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(repository[i].username, username) == 0 &&
            strcmp(repository[i].filename, filename) == 0) {
            found = true;
            remove_idx = i;
            break;
        }
    }

    if (found) {
        // 移动后续元素
        for (int i = remove_idx; i < entry_count - 1; i++) {
            repository[i] = repository[i + 1];
        }
        entry_count--;

        save_repository();
        sendto(sock, "The remove request was successful.", 34, 0, 
               (struct sockaddr*)client_addr, addr_len);
    } else {
        sendto(sock, "File not found.", 15, 0, 
               (struct sockaddr*)client_addr, addr_len);
    }
}

void cleanup() {
    if (repository) {
        free(repository);
        repository = NULL;
    }
}