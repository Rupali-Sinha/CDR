#include "process.h"

// Function to process customer billing data
void *process_customer_billing(void *arg) {
    FILE *in = fopen("data/data.txt", "r");  // Open input file for reading
    FILE *out = fopen("data/CB.txt", "w");   // Open output file for writing

    // Check if files opened successfully
    if (!in || !out) pthread_exit(NULL);

    Customer customers[MAX_CUSTOMERS];
    int customer_count = 0;
    char line[MAX_LINE];

    // Read and parse each line from the input file
    while (fgets(line, sizeof(line), in)) {
        char *fields[9];
        char *token = strtok(line, "|");
        int i = 0;
        
        // Tokenize the line into fields
        while (token && i < 9) {
            fields[i++] = token;
            token = strtok(NULL, "|");
        }
        if (i < 9) continue; // Ensure the required number of fields are present

        // Extract relevant data fields
        char *msisdn = fields[0];
        char *operator = fields[1];
        char *call_type = fields[3];
        int duration = atoi(fields[4]);
        int download = atoi(fields[5]);
        int upload = atoi(fields[6]);
        char *third_party_operator = fields[8];

        // Check if customer already exists in the array
        int idx = -1;
        for (int j = 0; j < customer_count; j++) {
            if (strcmp(customers[j].msisdn, msisdn) == 0) {
                idx = j;
                break;
            }
        }

        // If customer does not exist, create a new entry
        if (idx == -1) {
            idx = customer_count++;
            strcpy(customers[idx].msisdn, msisdn);
            strcpy(customers[idx].operator, operator);
            memset(&customers[idx].incoming_voice, 0, sizeof(Customer) - offsetof(Customer, incoming_voice));
        }

        // Categorize data based on call type
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

    // Write processed customer billing data to output file
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

    // Close files and exit thread
    fclose(in);
    fclose(out);
    pthread_exit(NULL);
}
