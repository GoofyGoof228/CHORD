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
#include "message.h"
#include <sys/select.h>
//#include "finger_table.h"
/**
 * must be filled at start up
 * list should be initialized with list create
 * */

struct _peer_info{

    bool first_peer;
    uint16_t self_id;
    uint32_t self_ip;
    uint16_t self_port;

    bool initialised_previous;
    uint16_t previous_id;
    uint32_t previous_ip;
    uint16_t previous_port;

    bool initialised_next;
    uint16_t next_id;
    uint32_t next_ip;
    uint16_t next_port;

    uint32_t join_ip;
    uint16_t join_port;

    payload** hash_head;
    payload** response_sockets_head;
    //TO DO two list for different types ???
    list* internal_states;
    list* external_states;
    void* ft;
    //fd_set* connection_storage;

};
typedef struct _peer_info peer_info;

bool join_is_done(peer_info* self);

int setup_peer_info(peer_info * self, char *argv[], int argc);

void print_addr_info(uint16_t id, uint32_t ip, uint32_t port);

void print_peer_info_short(peer_info* self);

void print_peer_info_long(peer_info* self);

char *peer_info_to_str(peer_info *self);

internal_message* pop_saved_state_int(list* states, uint16_t hash_id);

external_message* pop_saved_state_ext(list* states, uint16_t hash_id);

bool is_between(uint16_t hash, uint16_t prev, uint16_t now);

bool time_out(struct timeval *start);
#endif //BLOCK4_PEER_HELP_H