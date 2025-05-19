#ifndef POST_LOGIN_H
#define POST_LOGIN_H
 
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "utils.h"
 
void *process_customer_billing(void *arg) {
    int client_sock = *(int *)arg;
    send_msg(client_sock, "[Thread] Processing Customer Billing...\n");
    sleep(2);
    send_msg(client_sock, "[Thread] Customer Billing Done.\n");
    return NULL;
}
 
void *process_interoperator_billing(void *arg) {
    int client_sock = *(int *)arg;
    send_msg(client_sock, "[Thread] Processing Interoperator Billing...\n");
    sleep(2);
    send_msg(client_sock, "[Thread] Interoperator Billing Done.\n");
    return NULL;
}
 
void post_login_menu(int client_sock) {
    char buffer[1024];
    while (1) {
        send_msg(client_sock,
            "\n--- Post-Login Menu ---\n"
            "1. Process CDR File\n"
            "2. Print/Search Billing Info\n"
            "3. Logout\n"
            "Choice: ");
 
        memset(buffer, 0, sizeof(buffer));
        recv(client_sock, buffer, sizeof(buffer), 0);
        int choice = atoi(buffer);
 
        if (choice == 1) {
            pthread_t t1, t2;
            pthread_create(&t1, NULL, process_customer_billing, &client_sock);
            pthread_create(&t2, NULL, process_interoperator_billing, &client_sock);
            pthread_join(t1, NULL);
            pthread_join(t2, NULL);
        } else if (choice == 2) {
            send_msg(client_sock, "Billing Info: (Dummy Data)\n");
        } else if (choice == 3) {
            send_msg(client_sock, "Logging out...\n");
            break;
        } else {
            send_msg(client_sock, "Invalid choice.\n");
        }
    }
}
 
#endif
 
