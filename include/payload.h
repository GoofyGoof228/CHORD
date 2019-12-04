//
// Created by Artem Sereda on 06.11.19.
//

#ifndef BLOCK3_HELP_FUNCTIONS_H
#define BLOCK3_HELP_FUNCTIONS_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "uthash.h"

/**
 * wrappers struct for key, data, and some infos from header
 * */
struct _payload{
    uint8_t* key;
    uint16_t key_len;
    uint8_t * value;
    uint32_t val_len;
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct _payload payload;


/**
 * returns newly allocated array
 * */
uint8_t* array_deepcopy(uint8_t* old, uint32_t size);

/**
 * returns newly allocated struct payload
 * with given parameters
 * */
payload* create_payload(uint8_t * key, uint16_t key_len, uint8_t * value, uint32_t val_len);

/**
 * returns newly allocated deep copy
 * */
payload* payload_deepcopy(payload* old);

void free_payload(payload* old);

payload * ints_to_payload(uint32_t key, uint32_t value);

int buf_to_int(uint8_t *buf, uint16_t buf_len);

payload* create_empty_payload();
#endif //BLOCK3_HELP_FUNCTIONS_H
