//
// Created by Artem Sereda on 06.11.19.
//
#include <stdio.h>
#include <unistd.h>
#include "payload.h"

payload* create_empty_payload(){
    payload* p = calloc(1, sizeof(payload));
    p->key = calloc(0, sizeof(uint8_t));
    p->value = calloc(0, sizeof(uint8_t));
    return p;
}

uint8_t* array_deepcopy(uint8_t* old, uint32_t size){
    if(old == NULL){
        return NULL;
    }
    uint8_t* new_arr = calloc(size, sizeof(uint8_t));
    for(int i = 0; i != size; i++)new_arr[i] = old[i];
    return new_arr;
}

payload* create_payload(uint8_t * key, uint16_t key_len, uint8_t * value, uint32_t val_len){
    if(key == NULL || value == NULL){
        fprintf(stderr, "NUll pointer exception at create_payload() : key || value");
        return NULL;
    }
    payload* new_payload = malloc(sizeof(payload));
    new_payload->key_len = key_len;
    new_payload->val_len = val_len;
    new_payload->key =  array_deepcopy(key, key_len);
    new_payload->value = array_deepcopy(value, val_len);
    return new_payload;

}

payload* payload_deepcopy(payload* old){
    if(old == NULL){
        fprintf(stderr, "NUll pointer exception at payload_deepcopy() : old");
        return NULL;
    }
    payload *new_payload = malloc(sizeof(payload));
    new_payload->hh = old->hh;
    new_payload->key = array_deepcopy(old->key, old->key_len);
    new_payload->key_len = old->key_len;
    new_payload->val_len = old->val_len;
    new_payload->value = array_deepcopy(old->value, old->val_len);
    return new_payload;
}

payload * ints_to_payload(uint32_t key, uint32_t value){
    uint8_t key_buf[4];

    uint32_t b1 = (uint32_t) (key & (uint32_t) 0xff000000) >> (uint32_t) 24;
    uint32_t b2 = (uint32_t) (key & (uint32_t) 0x00ff0000) >> (uint32_t) 16;
    uint32_t b3 = (uint32_t) (key & (uint32_t) 0x0000ff00) >> (uint32_t) 8;
    uint32_t b4 = (uint32_t) (key & (uint32_t) 0x000000ff);

    key_buf[0] = (uint8_t) b1;
    key_buf[1] = (uint8_t) b2;
    key_buf[2] = (uint8_t) b3;
    key_buf[3] = (uint8_t) b4;

    uint8_t value_buf[4];

    b1 = (uint32_t) (value & (uint32_t) 0xff000000) >> (uint32_t) 24;
    b2 = (uint32_t) (value & (uint32_t) 0x00ff0000) >> (uint32_t) 16;
    b3 = (uint32_t) (value & (uint32_t) 0x0000ff00) >> (uint32_t) 8;
    b4 = (uint32_t) (value & (uint32_t) 0x000000ff);

    value_buf[0] = (uint8_t) b1;
    value_buf[1] = (uint8_t) b2;
    value_buf[2] = (uint8_t) b3;
    value_buf[3] = (uint8_t) b4;

    return create_payload(key_buf, 4, value_buf, 4);
}

int buf_to_int(uint8_t *buf, uint16_t buf_len){
    if(buf_len != 4){
        return -1;
    }
    uint32_t b1 = buf[0] << (uint8_t) 24;
    uint32_t b2 = buf[1] << (uint8_t) 16;
    uint32_t b3 = buf[2] << (uint8_t) 8;
    uint32_t b4 = buf[3];
    return  (int)(b1 | b2 | b3 | b4);
}

void free_payload(payload* old){
    if(old == NULL){
        fprintf(stderr, "NUll pointer exception at free_payload() : old");
        return;
    }
    if(old->key != NULL){
        free(old->key);
    }
    if(old->value != NULL){
        free(old->value);
    }
    free(old);
}



