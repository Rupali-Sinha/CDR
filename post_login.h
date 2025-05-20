#ifndef POST_LOGIN_H
#define POST_LOGIN_H
 
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "utils.h"

#define MAX_CUSTOMERS 1000
 
typedef struct {
    int customer_id;
    char operator[20];
    int in_voice_intra;
    int out_voice_intra;
    int in_sms_intra;
    int out_sms_intra;
    int in_voice_inter;
    int out_voice_inter;
    int in_sms_inter;
    int out_sms_inter;
    int mb_down;
    int mb_up;
} BillingInfo;
 
BillingInfo billing_data[MAX_CUSTOMERS];
int billing_count = 0;
 
BillingInfo* find_or_create_customer(int id, const char* operator) {
    for (int i = 0; i < billing_count; i++) {
        if (billing_data[i].customer_id == id) {
            return &billing_data[i];
        }
    }
    if (billing_count >= MAX_CUSTOMERS) return NULL;
 
    BillingInfo* b = &billing_data[billing_count++];
    b->customer_id = id;
    strncpy(b->operator, operator, sizeof(b->operator) - 1);
    b->operator[sizeof(b->operator) - 1] = '\0';
    b->in_voice_intra = b->out_voice_intra = 0;
    b->in_sms_intra = b->out_sms_intra = 0;
    b->in_voice_inter = b->out_voice_inter = 0;
    b->in_sms_inter = b->out_sms_inter = 0;
    b->mb_down = b->mb_up = 0;
    return b;
}
 
void *process_customer_billing(void *arg) {
    int client_sock = *(int *)arg;
    send_msg(client_sock, "[Thread] Processing Customer Billing...\n");
 
    FILE *cdr = fopen("cdr.txt", "r");
    if (!cdr) {
        send_msg(client_sock, "Error opening cdr.txt\n");
        return NULL;
    }
 
    char line[512];
    while (fgets(line, sizeof(line), cdr)) {
        char *tokens[10];
        int i = 0;
        char *token = strtok(line, "|");
        while (token && i < 10) {
            tokens[i++] = token;
            token = strtok(NULL, "|");
        }
 
        if (i < 9) continue;
 
        int caller_id = atoi(tokens[0]);
        char *operator = tokens[1];
        char *call_type = tokens[3];
        int duration = atoi(tokens[6]);
 
        if (!operator || !call_type) continue;
 
        BillingInfo* b = find_or_create_customer(caller_id, operator);
        if (!b) continue;
 
        if (strcmp(call_type, "MTC") == 0) {
            b->in_voice_intra += duration;
        } else if (strcmp(call_type, "MOC") == 0) {
            b->out_voice_intra += duration;
        } else if (strcmp(call_type, "SMS-MT") == 0) {
            b->in_sms_intra += 1;
        } else if (strcmp(call_type, "SMS-MO") == 0) {
            b->out_sms_intra += 1;
        } else if (strcmp(call_type, "GPRS") == 0) {
            b->mb_down += duration;
            b->mb_up += duration;
        }
    }
 
    fclose(cdr);
 
    FILE *cb = fopen("CB.txt", "w");
    if (!cb) {
        send_msg(client_sock, "Error creating CB.txt\n");
        return NULL;
    }
 
    for (int i = 0; i < billing_count; i++) {
        BillingInfo *b = &billing_data[i];
        fprintf(cb, "Customer ID: %d (%s)\n", b->customer_id, b->operator);
        fprintf(cb, "\t* Services within the mobile operator *\n");
        fprintf(cb, "\tIncoming voice call durations: %d\n", b->in_voice_intra);
        fprintf(cb, "\tOutgoing voice call durations: %d\n", b->out_voice_intra);
        fprintf(cb, "\tIncoming SMS messages: %d\n", b->in_sms_intra);
        fprintf(cb, "\tOutgoing SMS messages: %d\n", b->out_sms_intra);
        fprintf(cb, "\t* Services outside the mobile operator *\n");
        fprintf(cb, "\tIncoming voice call durations: %d\n", b->in_voice_inter);
        fprintf(cb, "\tOutgoing voice call durations: %d\n", b->out_voice_inter);
        fprintf(cb, "\tIncoming SMS messages: %d\n", b->in_sms_inter);
        fprintf(cb, "\tOutgoing SMS messages: %d\n", b->out_sms_inter);
        fprintf(cb, "\t* Internet use *\n");
        fprintf(cb, "\tMB downloaded: %d | MB uploaded: %d\n\n", b->mb_down, b->mb_up);
    }
 
    fclose(cb);
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
 
 
 
 
