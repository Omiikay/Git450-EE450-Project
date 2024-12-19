#include "utils.h"

// 存储用户凭证的结构
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];  // 已加密的密码
} UserCredential;

// 用户凭证数组
UserCredential* credentials = NULL;
int credential_count = 0;

// 函数声明
void load_credentials();
void handle_auth_request(int sock, struct sockaddr_in* client_addr);
int verify_credentials(const char* username, const char* password);
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
    server_addr.sin_port = htons(UDP_PORT_A);

    // 绑定socket
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        error("Error binding socket");
    }

    // 启动消息
    printf("Server A is up and running using UDP on port %d.\n", UDP_PORT_A);

    // 加载用户凭证
    load_credentials();

    // 主循环处理认证请求
    struct sockaddr_in client_addr;
    while (1) {
        handle_auth_request(sock, &client_addr);
    }

    // 清理（实际上不会执行到这里）
    cleanup();
    close(sock);
    return 0;
}

void load_credentials() {
    FILE* file = fopen("../data/members.txt", "r");
    if (!file) error("Error opening members.txt");

    char line[512];
    size_t capacity = 10;
    credential_count = 0;

    credentials = malloc(sizeof(UserCredential) * capacity);
    if (!credentials) {
        fclose(file);
        error("Memory allocation failed");
    }

    // 跳过标题行
    if (!fgets(line, sizeof(line), file)) {
        free(credentials);
        fclose(file);
        error("Empty or invalid members.txt file");
    }

    // 读取实际的用户数据
    while (fgets(line, sizeof(line), file)) {
        if (credential_count >= capacity) {
            capacity *= 2;
            UserCredential* temp = realloc(credentials, sizeof(UserCredential) * capacity);
            if (!temp) {
                free(credentials);
                fclose(file);
                error("Memory reallocation failed");
            }
            credentials = temp;
        }

        line[strcspn(line, "\n")] = 0;
        
        char* username = strtok(line, " ");
        char* password = strtok(NULL, " ");
        
        if (username && password) {
            strncpy(credentials[credential_count].username, username, sizeof(credentials[credential_count].username) - 1);
            strncpy(credentials[credential_count].password, password, sizeof(credentials[credential_count].password) - 1);
            credential_count++;
        }
    }

    if (credential_count < capacity) {
        UserCredential* temp = realloc(credentials, sizeof(UserCredential) * credential_count);
        if (temp) {
            credentials = temp;
        }
    }

    fclose(file);    
}

void handle_auth_request(int sock, struct sockaddr_in* client_addr) {
    char buffer[BUFFER_SIZE];
    char username[256], encrypted_password[256];
    socklen_t addr_len = sizeof(*client_addr);

    // 接收认证请求
    bzero(buffer, BUFFER_SIZE);
    recvfrom(sock, buffer, BUFFER_SIZE-1, 0,
             (struct sockaddr*)client_addr, &addr_len);

    // 解析认证请求
    if (sscanf(buffer, "AUTH %s %s", username, encrypted_password) != 2) {
        sendto(sock, "INVALID_REQUEST", 14, 0,
               (struct sockaddr*)client_addr, addr_len);
        return;
    }

    printf("ServerA received username %s and password ******\n", username);

    // 验证凭证
    int result = verify_credentials(username, encrypted_password);
    
    if (result == 0) {
        printf("Member %s has been authenticated\n", username);
        sendto(sock, "MEMBER_OK", 9, 0,
               (struct sockaddr*)client_addr, addr_len);
    } else {
        printf("The username %s or password ****** is incorrect\n", username);
        sendto(sock, "AUTH_FAILED", 11, 0,
               (struct sockaddr*)client_addr, addr_len);
    }
}

int verify_credentials(const char* username, const char* password) {
    for (int i = 0; i < credential_count; i++) {
        if (strcmp(credentials[i].username, username) == 0 &&
            strcmp(credentials[i].password, password) == 0) {
            return 0;  // 验证成功
        }
    }
    return -1;  // 验证失败
}

void cleanup() {
    if (credentials) {
        free(credentials);
        credentials = NULL;
    }
}