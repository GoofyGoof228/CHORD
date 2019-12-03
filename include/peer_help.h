//
// Created by Artem Sereda on 30.11.19.
//

#ifndef BLOCK4_PEER_HELP_H
#define BLOCK4_PEER_HELP_H

#include <stdint.h>
#include "payload.h"
#include "list.h"
#include "internal_message.h"
#include "external_message.h"
/**
 * must be filled at start up
 * list should be initialized with list create
 * */
struct _peer_info{

    uint16_t self_id;
    uint32_t self_ip;
    uint16_t self_port;

    uint16_t previous_id;
    uint32_t previous_ip;
    uint16_t previous_port;

    uint16_t next_id;
    uint32_t next_ip;
    uint16_t next_port;

    payload** hash_head;
    payload** response_sockets_head;
    list* states;

};
typedef struct _peer_info peer_info;

/**
* normal int will fit in buf with size 12
* */
void int_to_str(int n, char* str);

void print_addr_info(uint16_t id, uint32_t ip, uint32_t port);

void print_peer_info_short(peer_info* self);

void print_peer_info_long(peer_info* self);

internal_message* look_up_to_reply(internal_message* m_in, peer_info* self);

internal_message* create_reply(peer_info* self, uint16_t hash_id);

external_message* get_saved_state(list* states, uint16_t hash_id);

external_message* pop_saved_state(list* states, uint16_t hash_id);

/**
 * external should be not NULL
 * internal wiil be allocated in the function
 * must be freed afterwards
 * */
internal_message* create_look_up(external_message* m_ex, peer_info* self);

bool is_between(uint16_t hash, uint16_t prev, uint16_t now);
#endif //BLOCK4_PEER_HELP_H