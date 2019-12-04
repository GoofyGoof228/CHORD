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
typedef struct _message message;

bool is_internal(uint8_t byte);

int recv_message(message *m, int sock);

void free_message(message* old);


//TODO save meta message in list

#endif //BLOCK4_MESSAGE_H
