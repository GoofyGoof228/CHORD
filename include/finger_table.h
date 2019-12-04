//
// Created by Artem Sereda on 04.12.19.
//

#ifndef BLOCK5_FINGER_TABLE_H
#define BLOCK5_FINGER_TABLE_H
#include <stdint.h>
#include "peer_help.h"
#include "peer_netw.h"

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

uint16_t find_index(uint16_t id, finger_table* ft);

ft_entry* create_entry(uint16_t id, uint32_t ip, uint16_t port);

void create_ft(peer_info* self);

void init_fill_ft(peer_info* self);

void recieve_look_up(internal_message* lp, peer_info* self);

void refill_ft(peer_info* self);

void search_for_successor(uint16_t id, peer_info* self);

void free_ft(finger_table* ft);


#endif //BLOCK5_FINGER_TABLE_H
