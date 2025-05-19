#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
 
#define PORT 8080
#define MAX 1024
 
int is_prompt(const char *msg) {
    const char *prompts[] = {
        "Choice:", "Enter Username", "Enter Password"
    };
    for (int i = 0; i < 3; i++) {
        if (strstr(msg, prompts[i])) return 1;
    }
    return 0;
}
 
int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[MAX];
 
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
 
    while (1) {
        memset(buffer, 0, MAX);
        int bytes = recv(sock, buffer, MAX - 1, 0);
        if (bytes <= 0) break;
 
        buffer[bytes] = '\0';
        printf("%s", buffer);
 
        if (strstr(buffer, "Goodbye")) break;
 
        if (is_prompt(buffer)) {
            char input[MAX];
            fgets(input, MAX, stdin);
            input[strcspn(input, "\n")] = '\0'; // remove newline
            send(sock, input, strlen(input), 0);
        }
    }
 
    close(sock);
    return 0;
}
 
