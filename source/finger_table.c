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

uint16_t find_index(uint16_t id, finger_table* ft){
    for(int i = 0; i != ft->m; i++){
        if(ft->start_ids[i] == id) return i;
    }
    return -1;
}
ft_entry* create_entry(uint16_t id, uint32_t ip, uint16_t port){
    ft_entry* new = malloc(sizeof(ft_entry));
    new->id = id;
    new->ip = ip;
    new->port = port;
    return new;
}
finger_table* create_ft(uint32_t m, uint32_t n, peer_info* self){
    finger_table* ft_new = malloc(sizeof(finger_table));
    ft_new->m = m;
    ft_new->n = n;
    ft_new->start_ids = calloc(m, sizeof(uint16_t));
    ft_new->entries = calloc(m, sizeof(ft_entry*));
    return ft_new;
}
void fill_ft(peer_info* self){
    finger_table* ft = self->ft;
    uint16_t max_val = (uint16_t) powf( (float)2, (float) ft->m);
    for(int i = 0; i != ft->m; ++i){
        uint16_t start = ft->n + ((uint16_t)powf( (float)2, (float)i) ) % max_val;
        ft->start_ids[i] = start;
        search_for_successor(start, self);
    }
}
void refill_ft(peer_info* self){
    finger_table* ft = self->ft;

    for(int i = 0; i != ft->m; i++){
        free(ft->entries[i]);
    }
    fill_ft(self);
}

void recieve_look_up(internal_message* lp, peer_info* self){
    uint16_t index = find_index(lp->hash_id, self->ft);
    finger_table* ft = self->ft;
    ft->entries[index] = create_entry(lp->node_id, lp->node_ip, lp->node_port);

}
void search_for_successor(uint16_t id, peer_info* self){
    //TODO send "look up"
    internal_message* look_up = malloc(sizeof(internal_message));
    look_up->hash_id = id;
    look_up->node_id = self->self_id;
    look_up->node_ip = self->self_ip;
    look_up->node_port = self->self_port;
    look_up->type = LOOKUP;
    int next_peer_sock = connect_to_peer(self->next_ip, self->next_port);
    send_internal_message(look_up, next_peer_sock);
    close(next_peer_sock);

}


void free_ft(finger_table* ft){

    for(int i = 0; i != ft->m; i++){
        free(ft->entries[i]);
    }
    free(ft->entries);
    free(ft);
}