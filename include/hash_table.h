//
// Created by Artem Sereda on 05.11.19.
//
#ifndef BLOCK2_HASH_TABLE_H
#define BLOCK2_HASH_TABLE_H
#include <stdint.h>
#include "payload.h"
#include <stdio.h>
#define TEST
//#define H_DB

/**
 * returns new dep copy of structure,
 * whose key is the same as uint8_t* key
 * or NULL if key is not present in hash table
 **/
payload* h_get(payload **head, uint8_t* key, uint16_t key_len);
/**
 * adds deepcopy or new struct payload
 * to hash table, if key is not present
 * or rewrites value of old one to new
 * must be frees afterwards per hand
 **/
void h_set_p(payload** head, payload* to_set);
void h_set(payload **head, uint8_t* key, uint16_t key_len, uint8_t* value, uint32_t val_len);
/**
 * deletes the struct with given key, from table
 * frees the deleted from table element
 * if key is not present runtime error occurs
 **/
void h_del(payload** head, uint8_t *key, uint16_t key_len);
/**
 * deletes all elements from hash table,
 * in process also frees corresponding memory
 **/
void h_free(payload** head);

void set_hash_value(payload* p);

#endif //BLOCK2_HASH_TABLE_H
