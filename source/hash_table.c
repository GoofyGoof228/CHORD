//
// Created by Artem Sereda on 05.11.19.
//

#include "hash_table.h"


payload* h_get(payload **head, uint8_t* key, uint16_t key_len){

    if(head == NULL){
        fprintf(stderr, "Nullpointer exception at h_get : head");
        return NULL;
    }
    if(key == NULL){
        //fprintf(stderr, "Nullpointer exception at h_get : key");
        return NULL;
    }
    payload*p;
    HASH_FIND(hh, *head, key, key_len, p);
    #ifdef H_DB
    (p == NULL) ? printf("h_get() : Notfound %s\n", key) : printf("h_get() : found%s\n", key);
    #endif
    return (p != NULL) ? payload_deepcopy(p) : NULL;
}


void h_set_p(payload** head, payload* to_set){
    if(head == NULL){
        fprintf(stderr, "Nullpointer exception at h_set_p : head");
        return;
    }
    if(to_set == NULL){
        fprintf(stderr, "Nullpointer exception at h_set_p : to_set");
        return;
    }
    h_set(head, to_set->key, to_set->key_len, to_set->value, to_set->val_len);
}


void h_set(payload **head, uint8_t* key, uint16_t key_len, uint8_t* value, uint32_t val_len){
    if(head == NULL){
        fprintf(stderr, "Nullpointer exception at h_set : head");
        return;
    }
    if(key == NULL) {
        //fprintf(stderr, "Nullpointer exception at h_set : key");
        return;
    }
    if(value == NULL){
        if(h_get(head, key, key_len) == NULL){
            //fprintf(stderr, "Nullpointer exception at h_set : value");
            return;
        } else {
            //TO DO rewrite with empty value
            h_del(head, key, key_len);
            return;
        }
    }
    payload* p = NULL;
    HASH_FIND(hh, *head, key, key_len, p);

    #ifdef H_DB
    (p == NULL) ? printf("h_set() : Notfound %s\n", key) : printf("h_set() : found%s\n", key);
    #endif
    if(p == NULL){
        p = create_payload(key, key_len, value, val_len);
        HASH_ADD_KEYPTR(hh, *head, p->key, p->key_len, p);
    }else{
        p->val_len = val_len;
        for(int i = 0; i != val_len; i++)p->value[i] = value[i];
    }
}

void h_del(payload** head, uint8_t *key, uint16_t key_len) {
    if(head == NULL){
        fprintf(stderr, "Nullpointer exception at h_gel : head");
        return;
    }
    if(key == NULL){
        //fprintf(stderr, "Nullpointer exception at h_del : key");
        return;
    }
    payload* p;
    HASH_FIND(hh, *head, key, key_len, p);
    #ifdef H_DB
    (p == NULL) ? printf("h_del() : Notfound %s\n", key) : printf("h_del() : found%s\n", key);
    #endif
    if(p == NULL){
        //fprintf(stderr, "key not found");
        return;
    }
    HASH_DEL(*head, p);
    free_payload(p);
}

void h_free(payload **head){
    if(head == NULL){
        fprintf(stderr, "Nullpointer exception at h_free : head");
    }else if(*head == NULL)return;
    payload* iterator, *tmp;
    HASH_ITER(hh, *head, iterator, tmp) {
        HASH_DEL(*head, iterator);
        free_payload(iterator);
    }
}

void set_hash_value(payload* p){
    if(p == NULL){
        fprintf(stderr, "hash_code : p == NULL");
        return;
    }
    HASH_JEN(p->key, p->key_len, p->hh.hashv);
}