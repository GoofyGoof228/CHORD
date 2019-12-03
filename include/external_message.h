//
// Created by Jonathan Burggraf on 22.11.19.
//

#ifndef BLOCK4_EXTERNAL_MESSAGE_H
#define BLOCK4_EXTERNAL_MESSAGE_H

#include <stdbool.h>
#include <stdint.h>
#include "payload.h"

#define EXTERNAL_HEADER_LEN 7

/**
 * enum to identify command
 * */
enum external_action  {NOT_DEFINED, GET , SET, DELETE};
struct _external_message{
    enum external_action type;
    bool ack;
    payload *data;
    int socket_recieved_from;
};
typedef struct _external_message external_message;
/**
 * buf full buffer, from peer
 * msg - static declafred, all inner alloc will be done in func
 * all member fields will be allocated after func call
 * .data member must be freed after usage
 * per hand with free_payload()
 * after func returns, message struct contains
 * all needed data in human readable form (more or less)
 * */
int decode_external_header(uint8_t *buf, external_message *msg);
/**
 *buf - will be allocated in func
 * msg - pointer to static allocated
 * and filled "per hand" message struct
 * after func returns, buffer is filled
 * with header+value+key, and is ready to be sent
 * */
int encode_external_message(uint8_t *buf, external_message *msg);
/**
 * m - filled struct
 * */
int send_external_message(external_message *m, int client_sock);

void print_external_message(external_message *m);

void free_external_message(external_message* m);

external_message* create_empty_external_message(enum external_action to_do, bool b);

external_message* copy_external_message(external_message* m);
uint16_t get_hash_id(uint8_t* key, uint16_t key_len);
int compare_ext_messages(external_message* m, uint16_t* hash_id);
#endif //BLOCK4_EXTERNAL_MESSAGE_H
