//
// Created by Artem Sereda on 04.12.19.
//

#ifndef BLOCK5_FINGER_TABLE_H
#define BLOCK5_FINGER_TABLE_H
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#import <sys/socket.h>
#include <unistd.h>

#include "list.h"
#include "peer_help.h"
#include "peer_netw.h"
#define NUM_BITS_IN_HASH 16
#define TEST
#define SOCKET int
#define DG_FT
#define SOCKET int
#define FT_KEEP_ALIVE
#define SOCKET int
struct _ft_entry{
    uint16_t id;
    uint16_t port;
    uint32_t ip;
};
typedef struct _ft_entry ft_entry;

void print_entry(ft_entry* fe);


struct _finger_table{
    uint32_t m; //number of bits in hash id
    uint32_t n; //id of peer having this ft
    uint16_t* start_ids;
    ft_entry **entries;
    bool filled;
    SOCKET peer_who_asked_to_dew_it;


};
typedef struct _finger_table finger_table;


uint32_t powi(uint16_t base, uint16_t exp);
/**
 * m - number of bits in hash id
 * n - id of peer having this ft
 * */

bool ft_is_done(finger_table* ft);

uint16_t find_index(uint16_t id, finger_table* ft);

ft_entry* create_entry(uint16_t id, uint32_t ip, uint16_t port);

void create_ft(peer_info* self, SOCKET asked);

void init_fill_ft(peer_info* self);

void recieve_reply_ft(internal_message* lp, peer_info* self);

void send_ack(peer_info* self);

void search_for_successor(uint16_t id, peer_info* self);

void free_ft(finger_table* ft);

void print_ft(finger_table* ft);

void print_ft_in_file(finger_table* ft);

ft_entry* copy_entry(ft_entry* old);

ft_entry* find_corresponding_peer(finger_table* ft, uint16_t hash_id);

ft_entry* find_nearest_peer(finger_table* ft, uint16_t hash_id);

ft_entry* get_last_entry(finger_table* ft);

#endif //BLOCK5_FINGER_TABLE_H
