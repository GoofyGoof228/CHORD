//
// Created by Jonathan Burggraf on 21.11.19.
//

#ifndef BLOCK4_MESSAGE_H
#define BLOCK4_MESSAGE_H

#include <stdbool.h>
#include "internal_message.h"
#include "external_message.h"
/**
 * "meta" message
 * */
struct _message {
    bool internal;
    external_message *ext_msg;
    internal_message *int_msg;
};

bool is_internal(uint8_t byte);
typedef struct _message message;
int check_socket(int socket);
int recv_message(message *m, int sock);
int send_message(message* m, int sock);
void free_message(message* old);
#endif //BLOCK4_MESSAGE_H
