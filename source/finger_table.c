//
// Created by Artem Sereda on 04.12.19.
//

#include "finger_table.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "peer_netw.h"
#import <sys/socket.h>
#include <unistd.h>
#include "list.h"
#define NUM_BITS_IN_HASH 16
#ifndef TEST
#define TEST
#endif
uint16_t find_index(uint16_t id, finger_table* ft){
    for(int i = 0; i != ft->m; i++){
        if(ft->start_ids[i] == id) return i;
    }
    return -1;
}
bool ft_is_done(finger_table* ft){
    for(int i = 0; i != ft->m; ++i){
        if(ft->entries[i] == NULL){
            ft->filled = false;
            return ft->filled;
        }
    }
    ft->filled = true;
    return ft->filled;
}
ft_entry* create_entry(uint16_t id, uint32_t ip, uint16_t port){
    ft_entry* new = malloc(sizeof(ft_entry));
    new->id = id;
    new->ip = ip;
    new->port = port;
    return new;
}
void create_ft(peer_info* self){
    finger_table* ft_new = malloc(sizeof(finger_table));
    ft_new->m = NUM_BITS_IN_HASH;
    ft_new->n = self->self_id;
    ft_new->start_ids = calloc(NUM_BITS_IN_HASH, sizeof(uint16_t));
    ft_new->entries = calloc(NUM_BITS_IN_HASH, sizeof(ft_entry*));
    for(int i = 0; i!= NUM_BITS_IN_HASH; ++i)ft_new->entries[i] = NULL;
    ft_new->filled = false;
    self->ft = ft_new;
}
void init_fill_ft(peer_info* self){
    finger_table* ft = self->ft;
    uint32_t max_val = (uint32_t) powf( (float)2, (float) ft->m);
    for(int i = 0; i != ft->m; ++i){
        uint16_t start = ft->n + ((uint16_t)powf( (float)2, (float)i) ) % max_val;
        ft->start_ids[i] = start;
        if(is_between(ft->start_ids[i], self->previous_id, self->self_id)){
            ft->entries[i] = create_entry(self->self_id, self->self_ip, self->self_port);
            continue;
        }
        if(is_between(ft->start_ids[i], self->self_id, self->next_id)){
            ft->entries[i] = create_entry(self->next_id, self->next_ip, self->next_port);
            continue;
        }
        search_for_successor(start, self);
        message* in;
        //recv_message(&in, )
    }
}
void refill_ft(peer_info* self){
    finger_table* ft = self->ft;

    for(int i = 0; i != ft->m; i++){
        free(ft->entries[i]);
    }
    //fill_ft(self);
}

void recieve_reply_ft(internal_message* lp, peer_info* self){
    //TODO which flag should be set, to recongnise ft lookup answer ???
    int index = find_index(lp->hash_id, (finger_table*) self->ft);
    finger_table *ft = self->ft;
    ft->entries[index] = create_entry(lp->node_id, lp->node_ip, lp->node_port);
    ft_is_done(ft);
}
void search_for_successor(uint16_t id, peer_info* self){
    //TODO send "look up"
    internal_message* look_up = malloc(sizeof(internal_message));
    look_up->hash_id = id;
    look_up->node_id = self->self_id;
    look_up->node_ip = self->self_ip;
    look_up->node_port = self->self_port;
    look_up->type = LOOKUP;

    listPushBack(self->states, create_wrapper(look_up, INTERNAL_MES));
    int next_peer_sock = connect_to_peer(self->next_ip, self->next_port);
    send_internal_message(look_up, next_peer_sock);
    //free(look_up);
    close(next_peer_sock);
    //return next_peer_sock;

}


void free_ft(finger_table* ft){

    for(int i = 0; i != ft->m; i++){
        free(ft->entries[i]);
    }
    free(ft->entries);
    free(ft);
}

void print_ft(finger_table* ft){
    if(ft == NULL){
        fprintf(stderr, "ft == NULL\n");
        return;
    }
    printf("i     start[i]     ft[i]\n");
    printf("-------------------------\n");
    for(int i = 0; i != ft->m; i++){
        printf("%d    %d    ", i, ft->start_ids[i]);
        ft->entries[i] == NULL ? printf("NULL\n") : printf("%d\n", ft->entries[i]->id);
    }
}