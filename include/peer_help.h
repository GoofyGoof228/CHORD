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
    list* states;
    void* ft;

};
typedef struct _peer_info peer_info;

int setup_peer_info(peer_info * self, char *argv[], int argc);

void print_addr_info(uint16_t id, uint32_t ip, uint32_t port);

void print_peer_info_short(peer_info* self);

void print_peer_info_long(peer_info* self);

message* get_saved_state(list* states, uint16_t hash_id, const int type);

message* pop_saved_state(list* states, uint16_t hash_id, const int type);

bool is_between(uint16_t hash, uint16_t prev, uint16_t now);

/**finget tabele**/

struct _ft_entry{
    uint16_t id;
    uint16_t port;
    uint32_t ip;
};
typedef struct _ft_entry ft_entry;

struct _finger_table{
    uint32_t m; //number of bits in hash id
    uint32_t n; //id of peer having this ft
    uint16_t* start_ids;
    ft_entry **entries;
    bool filled;

};
typedef struct _finger_table finger_table;
/**
 * m - number of bits in hash id
 * n - id of peer having this ft
 * */

bool ft_is_done(finger_table* ft);

uint16_t find_index(uint16_t id, finger_table* ft);

ft_entry* create_entry(uint16_t id, uint32_t ip, uint16_t port);

void create_ft(peer_info* self);

void init_fill_ft(peer_info* self);

void recieve_reply_ft(internal_message* lp, peer_info* self);

void refill_ft(peer_info* self);

void search_for_successor(uint16_t id, peer_info* self);

void free_ft(finger_table* ft);

void print_ft(finger_table* ft);

#endif //BLOCK4_PEER_HELP_H