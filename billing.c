#include "process.h"

void *process_customer_billing(void *arg) {
    FILE *in = fopen("data/data.txt", "r");
    FILE *out = fopen("data/CB.txt", "w");

    if (!in || !out) pthread_exit(NULL);

    Customer customers[MAX_CUSTOMERS];
    int customer_count = 0;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), in)) {
        char *fields[9];
        char *token = strtok(line, "|");
        int i = 0;
        while (token && i < 9) {
            fields[i++] = token;
            token = strtok(NULL, "|");
        }
        if (i < 9) continue;

        char *msisdn = fields[0];
        char *operator = fields[1];
        char *call_type = fields[3];
        int duration = atoi(fields[4]);
        int download = atoi(fields[5]);
        int upload = atoi(fields[6]);
        char *third_party_operator = fields[8];

        int idx = -1;
        for (int j = 0; j < customer_count; j++) {
            if (strcmp(customers[j].msisdn, msisdn) == 0) {
                idx = j;
                break;
            }
        }

        if (idx == -1) {
            idx = customer_count++;
            strcpy(customers[idx].msisdn, msisdn);
            strcpy(customers[idx].operator, operator);
            memset(&customers[idx].incoming_voice, 0, sizeof(Customer) - offsetof(Customer, incoming_voice));
        }

        if (strcmp(call_type, "MTC") == 0) {
            if (strcmp(operator, third_party_operator) == 0)

                customers[idx].incoming_voice += duration;
            else
                customers[idx].incoming_voice_other += duration;
        } else if (strcmp(call_type, "MOC") == 0) {
            if (strcmp(operator, third_party_operator) == 0)
                customers[idx].outgoing_voice += duration;
            else
                customers[idx].outgoing_voice_other += duration;
        } else if (strcmp(call_type, "SMS-MT") == 0) {
            if (strcmp(operator, third_party_operator) == 0)
                customers[idx].incoming_sms += 1;
            else
                customers[idx].incoming_sms_other += 1;
        } else if (strcmp(call_type, "SMS-MO") == 0) {
            if (strcmp(operator, third_party_operator) == 0)
                customers[idx].outgoing_sms += 1;
            else
                customers[idx].outgoing_sms_other += 1;
        } else if (strcmp(call_type, "GPRS") == 0) {
            customers[idx].download += download;
            customers[idx].upload += upload;
        }
    }

    fprintf(out, "# Customers Data Base:\n");
    for (int i = 0; i < customer_count; i++) {
        Customer *c = &customers[i];
        fprintf(out, "Customer ID: %s (%s)\n", c->msisdn, c->operator);
        fprintf(out, "\t* Services within the mobile operator *\n");
        fprintf(out, "\tIncoming voice call durations: %d\n", c->incoming_voice);
        fprintf(out, "\tOutgoing voice call durations: %d\n", c->outgoing_voice);
        fprintf(out, "\tIncoming SMS messages: %d\n", c->incoming_sms);
        fprintf(out, "\tOutgoing SMS messages: %d\n", c->outgoing_sms);
        fprintf(out, "\t* Services outside the mobile operator *\n");
        fprintf(out, "\tIncoming voice call durations: %d\n", c->incoming_voice_other);
        fprintf(out, "\tOutgoing voice call durations: %d\n", c->outgoing_voice_other);
        fprintf(out, "\tIncoming SMS messages: %d\n", c->incoming_sms_other);
        fprintf(out, "\tOutgoing SMS messages: %d\n", c->outgoing_sms_other);
        fprintf(out, "\t* Internet use *\n");
        fprintf(out, "\tMB downloaded: %d | MB uploaded: %d\n\n", c->download, c->upload);
    }

    fclose(in);
    fclose(out);
    pthread_exit(NULL);

}

void *process_operator_billing(void *arg) {
    FILE *in = fopen("data/data.txt", "r");
    FILE *out = fopen("data/IOCB.txt", "w");

    if (!in || !out) pthread_exit(NULL);

    Operator operators[MAX_OPERATORS];
    int operator_count = 0;
    char line[MAX_LINE];

while (fgets(line, sizeof(line), in)) {
        char *fields[9];
        char *token = strtok(line, "|");
        int i = 0;
        while (token && i < 9) {
            fields[i++] = token;
            token = strtok(NULL, "|");
        }
        if (i < 9) continue;
char *operator = fields[1];
        char *mccmnc = fields[2];
        char *call_type = fields[3];
        int duration = atoi(fields[4]);
        int download = atoi(fields[5]);
        int upload = atoi(fields[6]);

        int idx = -1;
        for (int j = 0; j < operator_count; j++) {
            if (strcmp(operators[j].mccmnc, mccmnc) == 0) {
                idx = j;
                break;
            }
        }
  if (idx == -1) {
            idx = operator_count++;
            strcpy(operators[idx].operator, operator);
            strcpy(operators[idx].mccmnc, mccmnc);
            memset(&operators[idx].incoming_voice, 0, sizeof(Operator) - offsetof(Operator, incoming_voice));
        }

if (strcmp(call_type, "MTC") == 0) {
            operators[idx].incoming_voice += duration;
        } else if (strcmp(call_type, "MOC") == 0) {
            operators[idx].outgoing_voice += duration;

operators[idx].outgoing_voice += duration;
        } else if (strcmp(call_type, "SMS-MT") == 0) {
            operators[idx].incoming_sms += 1;
        } else if (strcmp(call_type, "SMS-MO") == 0) {
            operators[idx].outgoing_sms += 1;
        } else if (strcmp(call_type, "GPRS") == 0) {
            operators[idx].download += download;
            operators[idx].upload += upload;
        }
    }

   fprintf(out, "# Operator Data Base:\n");
    for (int i = 0; i < operator_count; i++) {
        Operator *op = &operators[i];
        fprintf(out, "Operator Brand: %s (%s)\n", op->operator, op->mccmnc);
        fprintf(out, "\tIncoming voice call durations: %d\n", op->incoming_voice);
        fprintf(out, "\tOutgoing voice call durations: %d\n", op->outgoing_voice);
        fprintf(out, "\tIncoming SMS messages: %d\n", op->incoming_sms);
        fprintf(out, "\tOutgoing SMS messages: %d\n", op->outgoing_sms);
        fprintf(out, "\tMB downloaded: %d | MB uploaded: %d\n\n", op->download, op->upload);
    }

fclose(in);
    fclose(out);
    pthread_exit(NULL);
}

void process_cdr_file(int client_sock) {
    pthread_t customer_thread, operator_thread;

    pthread_create(&customer_thread, NULL, process_customer_billing, NULL);
    pthread_create(&operator_thread, NULL, process_operator_billing, NULL);

    pthread_join(customer_thread, NULL);
    pthread_join(operator_thread, NULL);

send(client_sock, "CDR processing complete. Output saved to CB.txt and IOCB.txt\n", 61, 0);
}

void print_search_billing_info(int client_sock) {
    const char *msg = "Printing/Searching Billing Information...\n";
    send(client_sock, msg, strlen(msg), 0);
}

void post_login_menu(int client_sock) {
    char buffer[MAX];
    int choice;

 while (1) {
        const char *menu =
            "\n--- Post-Login Menu ---\n"
            "1. Process CDR file\n"
            "2. Print/Search for Billing Information\n"
            "3. Logout\n"
            "Choice: ";
        send(client_sock, menu, strlen(menu), 0);

  memset(buffer, 0, MAX);
        recv(client_sock, buffer, MAX, 0);
        choice = atoi(buffer);

 switch (choice) {
            case 1:
                process_cdr_file(client_sock);
                break;
            case 2:
                print_search_billing_info(client_sock);
                break;
            case 3:
                send(client_sock, "Logging out...\n", 15, 0);
                return;
            default:
                send(client_sock, "Invalid choice. Please try again.\n", 34, 0);
        }
    }
}
this is my process.c  now when i give choice 2 then display these option and make seperate file billing.c and billing, h and link it 
