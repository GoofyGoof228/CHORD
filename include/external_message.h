//
// Created by Jonathan Burggraf on 22.11.19.
//

#ifndef BLOCK4_EXTERNAL_MESSAGE_H
#define BLOCK4_EXTERNAL_MESSAGE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
//#define TEST
#include "payload.h"


#define EXTERNAL_HEADER_LEN 7

/**
 * enum to identify command
 * */
enum external_action  {NOT_DEFINED, GET , SET, DELETE};
typedef enum external_action external_action;
struct _external_message{
    enum external_action type;
    bool ack;
    payload *data;
    int socket_recieved_from;
};
typedef struct _external_message external_message;

int encode_external_message(uint8_t *buf, external_message *msg);

int decode_external_header(uint8_t *buf, external_message *msg);

int send_external_message(external_message *m, int client_sock);

void free_external_message(external_message* m);

void print_external_message(external_message *m);

uint16_t get_hash_id(uint8_t* key, uint16_t key_len);

#endif //BLOCK4_EXTERNAL_MESSAGE_H
