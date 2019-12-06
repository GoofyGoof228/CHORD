//
// Created by Jonathan Burggraf on 21.11.19.
//

#ifndef BLOCK4_MESSAGE_H
#define BLOCK4_MESSAGE_H

#include <stdbool.h>
#include "internal_message.h"
#include "external_message.h"
#define INTERNAL_MES -17
#define EXTERNAL_MES  -21
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

message* create_wrapper(void* m, const int type);
//TODO save meta message in list

void print_message(message* m);
#endif //BLOCK4_MESSAGE_H
