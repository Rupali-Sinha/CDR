#ifndef UTILS_H
#define UTILS_H
 
#include <string.h>
#include <unistd.h>
 
void send_msg(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}
 
#endif
 
