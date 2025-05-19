#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "utils.h"
#include "post_login.h"
 
#define PORT 8080
#define MAX 1024
#define USER_FILE "users.txt"
 
int is_valid_password(const char *pass) {
    int length = 0, has_upper = 0, has_lower = 0, has_num = 0, has_special = 0;
    char special_chars[] = "!@#$%^&()_-+=<>?/";
 
    for (int i = 0; pass[i]; i++) {
        length++;
        if (isupper(pass[i])) has_upper = 1;
        else if (islower(pass[i])) has_lower = 1;
        else if (isdigit(pass[i])) has_num = 1;
        else if (strchr(special_chars, pass[i])) has_special = 1;
    }
 
    return (length >= 8 && has_upper && has_lower && has_num && has_special);
}
 
int user_exists(const char *username) {
    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) return 0;
    char u[100], p[100];
    while (fscanf(fp, "%s %s", u, p) != EOF) {
        if (strcmp(username, u) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}
 
int verify_login(const char *username, const char *password) {
    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) return 0;
    char u[100], p[100];
    while (fscanf(fp, "%s %s", u, p) != EOF) {
        if (strcmp(username, u) == 0 && strcmp(password, p) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}
 
void handle_client(int client_sock) {
    char buffer[MAX];
    while (1) {
        send_msg(client_sock,
            "\n--- Main Menu ---\n"
            "1. SignUp\n"
            "2. Login\n"
            "3. Exit\n"
            "Choice: ");
 
        memset(buffer, 0, MAX);
        recv(client_sock, buffer, MAX, 0);
        int choice = atoi(buffer);
 
        if (choice == 1) {
            char username[100], password[100];
 
            send_msg(client_sock, "Enter Username: ");
            recv(client_sock, username, sizeof(username), 0);
            username[strcspn(username, "\n")] = 0;
 
            if (user_exists(username)) {
                send_msg(client_sock, "Username already exists.\n");
                continue;
            }
 
            send_msg(client_sock, "Enter Password (min 8 chars, upper, lower, number, special): ");
            recv(client_sock, password, sizeof(password), 0);
            password[strcspn(password, "\n")] = 0;
 
            if (!is_valid_password(password)) {
                send_msg(client_sock, "Invalid password format.\n");
                continue;
            }
 
            FILE *fp = fopen(USER_FILE, "a");
            fprintf(fp, "%s %s\n", username, password);
            fclose(fp);
            send_msg(client_sock, "SignUp successful!\n");
 
        } else if (choice == 2) {
            char username[100], password[100];
 
            send_msg(client_sock, "Enter Username: ");
            recv(client_sock, username, sizeof(username), 0);
            username[strcspn(username, "\n")] = 0;
 
            send_msg(client_sock, "Enter Password: ");
            recv(client_sock, password, sizeof(password), 0);
            password[strcspn(password, "\n")] = 0;
 
            if (verify_login(username, password)) {
                send_msg(client_sock, "Login successful!\n");
                post_login_menu(client_sock);
            } else {
                send_msg(client_sock, "Invalid username or password.\n");
            }
 
        } else if (choice == 3) {
            send_msg(client_sock, "Goodbye!\n");
            break;
        } else {
            send_msg(client_sock, "Invalid choice.\n");
        }
    }
 
    close(client_sock);
}
 
int main() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
 
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
 
    printf("Server listening on port %d...\n", PORT);
    client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    printf("Client connected.\n");
 
    handle_client(client_sock);
    close(server_fd);
    return 0;
}
 
