//
// Created by Jonathan Burggraf on 21.11.19.
//

#ifndef BLOCK4_INTERNAL_MESSAGE_H
#define BLOCK4_INTERNAL_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>
#define INTERNAL_HEADER_LEN 11

enum internal_action  {FINGER, F_ACK, JOIN, NOTIFY, STABILIZE, REPLY, LOOKUP};

struct _internal_message{
    enum internal_action type;
    uint16_t hash_id;
    uint16_t node_id;
    uint32_t node_ip;
    uint32_t node_port;
};
typedef struct _internal_message internal_message;

void print_internal_message(internal_message* m);

int encode_internal_message(uint8_t *buf, internal_message *m);

int decode_internal_header(uint8_t *buf, internal_message *m);

int send_internal_message(internal_message *m, int sock);

internal_message* copy_int_message(internal_message* old);
#endif //BLOCK4_INTERNAL_MESSAGE_H
