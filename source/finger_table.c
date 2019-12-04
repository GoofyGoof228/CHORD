//
// Created by Artem Sereda on 04.12.19.
//

#include "finger_table.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

finger_table* create_ft(uint32_t m, uint32_t n, peer_info* self){
    finger_table* ft_new = malloc(sizeof(finger_table));
    ft_new->m = m;
    ft_new->n = n;
    ft_new->entries = calloc(m, sizeof(ft_entry*));
}
void fill_ft(peer_info* self){
    finger_table* ft = self->ft;
    uint16_t max_val = (uint16_t) pow( (float)2, (float) ft->m);
    for(int i = 0; i != ft->m; ++i){
        uint16_t start = ft->n + ((uint16_t)pow( (float)2, (float)i) ) % max_val;
        ft->entries[i] = find_successor(start);
    }
}
void refill_ft(peer_info* self){
    finger_table* ft = self->ft;

    for(int i = 0; i != ft->m; i++){
        free(ft->entries[i]);
    }
    fill_ft(self);
}
ft_entry* find_peer(uint16_t id, peer_info* self){
    finger_table* ft = self->ft;
    for(int i = ft->m; i >= 0; --i){

    }
}

ft_entry* find_successor(uint16_t id){
    //TODO send "look up"

}


void free_ft(finger_table* ft){

    for(int i = 0; i != ft->m; i++){
        free(ft->entries[i]);
    }
    free(ft->entries);
    free(ft);
}