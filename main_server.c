 // File: server.c
// Description: Multi-client threaded server handling login, signup, and exit with SHA-256 hashing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define FILENAME "users.txt"
#define SALT "somesalt" // Example salt

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// SHA-256 hashing
void sha256_hash(const char *input, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, strlen(input));
    SHA256_Update(&sha256, SALT, strlen(SALT));
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = 0;
}

// Thread function to handle each client
void *handle_client(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        // Send menu to client
        char menu[] = "\n================ MENU ================\n1. Signup\n2. Login\n3. Exit\nChoose an option: ";
        if (send(client_sock, menu, strlen(menu), 0) < 0) {
            perror("[SERVER]: Failed to send menu");
            break;
        }

        // Receive option
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(client_sock, buffer, sizeof(buffer), 0);
        if (valread <= 0) break;
        buffer[valread] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "3") == 0) {
            send(client_sock, "\n[SERVER]: Goodbye!\n", strlen("\n[SERVER]: Goodbye!\n"), 0);
            break;
        }

        char option[10], username[100], password[100];
        strcpy(option, buffer);

        // Ask for username
        send(client_sock, "\n[SERVER]: Enter username: ", strlen("\n[SERVER]: Enter username: "), 0);
        valread = recv(client_sock, username, sizeof(username), 0);
        if (valread <= 0) break;
        username[valread] = '\0';
        username[strcspn(username, "\n")] = 0;

        // Ask for password
        send(client_sock, "[SERVER]: Enter password: ", strlen("[SERVER]: Enter password: "), 0);
        valread = recv(client_sock, password, sizeof(password), 0);
        if (valread <= 0) break;
        password[valread] = '\0';
        password[strcspn(password, "\n")] = 0;

        char hashed_password[65];
        sha256_hash(password, hashed_password);

        if (strcmp(option, "1") == 0) {
            // Signup
            pthread_mutex_lock(&file_mutex);
            FILE *fp = fopen(FILENAME, "a+");
            if (!fp) {
                perror("[SERVER]: Failed to open file");
                pthread_mutex_unlock(&file_mutex);
                continue;
            }

            char line[256], user[100];
            int exists = 0;

            while (fgets(line, sizeof(line), fp)) {
                sscanf(line, "%s", user);
                if (strcmp(user, username) == 0) {
                    exists = 1;
                    break;
                }
            }

            if (exists) {
                strcpy(response, "[SERVER]: Username already exists.\n");
            } else {
                fprintf(fp, "%s %s\n", username, hashed_password);
                strcpy(response, "[SERVER]: Signup successful.\n");
            }

            fclose(fp);
            pthread_mutex_unlock(&file_mutex);

        } else if (strcmp(option, "2") == 0) {
            // Login
            pthread_mutex_lock(&file_mutex);
            FILE *fp = fopen(FILENAME, "r");
            if (!fp) {
                perror("[SERVER]: Failed to open file");
                pthread_mutex_unlock(&file_mutex);
                continue;
            }

            char line[256], user[100], pass[100];
            int found = 0;

            while (fgets(line, sizeof(line), fp)) {
                sscanf(line, "%s %s", user, pass);
                if (strcmp(user, username) == 0 && strcmp(pass, hashed_password) == 0) {
                    found = 1;
                    break;
                }
            }

            fclose(fp);
            pthread_mutex_unlock(&file_mutex);

            if (found) {
                strcpy(response, "[SERVER]: Login successful.\n");
            } else {
                strcpy(response, "[SERVER]: Invalid username or password.\n");
            }

        } else {
            strcpy(response, "[SERVER]: Invalid option.\n");
        }

        send(client_sock, response, strlen(response), 0);
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main() {
    int server_fd, *client_sock;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("[SERVER]: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[SERVER]: Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("[SERVER]: Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[SERVER]: Server running on port %d. Waiting for clients...\n", PORT);

    while (1) {
        client_sock = malloc(sizeof(int));
        if (!client_sock) {
            perror("[SERVER]: Memory allocation failed");
            continue;
        }

        *client_sock = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if (*client_sock < 0) {
            perror("[SERVER]: Accept failed");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)client_sock) != 0) {
            perror("[SERVER]: Thread creation failed");
            free(client_sock);
        } else {
            pthread_detach(tid);
        }
    }

    close(server_fd);
    return 0;
}
