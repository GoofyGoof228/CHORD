//
// Created by Artem Sereda on 04.12.19.
//

#ifndef BLOCK5_FINGER_TABLE_H
#define BLOCK5_FINGER_TABLE_H
#include <stdint.h>
struct _ft_entry{
    uint16_t id;
    uint16_t port;
    uint32_t ip;
};
typedef struct _ft_entry ft_entry;

struct _finger_table{
    uint32_t m; //number of bits in hash id
    uint32_t n; //id of peer having this ft
    ft_entry *entries;

};
typedef struct _finger_table finger_table;
/**
 * m - number of bits in hash id
 * n - id of peer having this ft
 * */
finger_table* create_ft(uint32_t m, uint32_t n);
void refill_ft(finger_table* ft);
ft_entry* find_peer(uint16_t id, finger_table* ft);

ft_entry* find_successor(uint16_t id);
ft_entry* find_id(uint16_t id);
#endif //BLOCK5_FINGER_TABLE_H
