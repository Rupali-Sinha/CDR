#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to validate password
int is_valid_password(const char *password) {
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    for (int i = 0; password[i] != '\0'; i++) {
        if (isupper(password[i])) has_upper = 1;
        else if (islower(password[i])) has_lower = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else has_special = 1;
    }
    return has_upper && has_lower && has_digit && has_special;
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[CLIENT]: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CLIENT]: Connection to server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[CLIENT]: Connected to server at port %d\n", PORT);

    while (1) {
        // Receive menu
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            printf("[CLIENT]: Server closed the connection.\n");
            break;
        }
        buffer[valread] = '\0';
        printf("%s", buffer);

        // Send option
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(sock, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "3") == 0) {
            memset(response, 0, sizeof(response));
            recv(sock, response, sizeof(response) - 1, 0);
            printf("%s\n", response);
            break;
        }

        // Receive username prompt
        memset(buffer, 0, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer) - 1, 0);
        printf("%s", buffer);
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(sock, buffer, strlen(buffer), 0);

        // Receive password prompt
        memset(buffer, 0, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer) - 1, 0);
        printf("%s", buffer);

        // Get and validate password
        do {
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            if (!is_valid_password(buffer)) {
                printf("[CLIENT]: Password must contain at least one uppercase letter, one lowercase letter, one digit, and one special character.\n");
                printf("Please enter password again: ");
            }
        } while (!is_valid_password(buffer));

        send(sock, buffer, strlen(buffer), 0);

        // Receive server response
        memset(response, 0, sizeof(response));
        valread = recv(sock, response, sizeof(response) - 1, 0);
        if (valread <= 0) break;
        response[valread] = '\0';
        printf("\n%s\n", response);

        // Handle signup success and redirect to login
        if (strstr(response, "Signup successful")) {
            printf("[CLIENT]: Redirecting to login...\n");
            strcpy(buffer, "2"); // Assuming "2" is the login option
            send(sock, buffer, strlen(buffer), 0);
            continue;
        }

        // Handle login success and return to menu
        if (strstr(response, "Login successful")) {
            printf("[CLIENT]: Login successful. Press Enter to return to menu...\n");
            fgets(buffer, sizeof(buffer), stdin);
            continue;
        }

        // Otherwise, return to menu
        printf("[CLIENT]: Press Enter to return to menu...\n");
        fgets(buffer, sizeof(buffer), stdin);
        usleep(200000); // Optional delay
    }

    close(sock);
    return 0;
}

