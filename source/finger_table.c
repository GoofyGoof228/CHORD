//
// Created by Artem Sereda on 04.12.19.
//

#include "finger_table.h"
#include <stdlib.h>
//#include <math.h>
#include <stdint.h>
#include "peer_netw.h"
#import <sys/socket.h>
#include <unistd.h>
#include "list.h"
#include <string.h>
#define NUM_BITS_IN_HASH 16
#define TEST


uint32_t powi(uint16_t base, uint16_t exp){
    if(exp == 0) return 1;
    return base * powi(base, exp-1);
}
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
void create_ft(peer_info* self, int socket){
    finger_table* ft_new = malloc(sizeof(finger_table));
    ft_new->m = NUM_BITS_IN_HASH;
    ft_new->n = self->self_id;
    ft_new->start_ids = calloc(NUM_BITS_IN_HASH, sizeof(uint16_t));
    ft_new->entries = calloc(NUM_BITS_IN_HASH, sizeof(ft_entry*));
    for(int i = 0; i!= NUM_BITS_IN_HASH; ++i)ft_new->entries[i] = NULL;
    ft_new->filled = false;
    self->ft = ft_new;
    ft_new->socket_asked_to_dew_it = socket;
}
void init_fill_ft(peer_info* self){
    finger_table* ft = self->ft;
    uint32_t max_val =  powi( 2, ft->m);
    for(int i = 0; i != ft->m; ++i){
        uint16_t start = ft->n + powi( 2, i) % max_val;
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
    if(ft_is_done(ft)){
        //case for commando-line
        if(((finger_table*)self->ft)->socket_asked_to_dew_it == -1)return;
        //TODO send ack
        internal_message out;
        out.hash_id = ((finger_table*)self->ft)->n;
        out.node_id = self->self_id;
        out.node_ip = self->self_ip;
        out.node_port = self->self_port;
        send_internal_message(&out, ((finger_table*)self->ft)->socket_asked_to_dew_it);
        close(((finger_table*)self->ft)->socket_asked_to_dew_it);
    }
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
    free(ft->start_ids);
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

void print_ft_in_file(finger_table *ft){
    if(!ft->filled)return;
    char file_name[30];
    snprintf(file_name, 30, "finger_table_id=%d.log", ft->n);
    FILE* table_log = fopen(file_name, (const char *) 'w');
    if(table_log == NULL){
        fprintf (stderr, "Couldn't open file %s\n", file_name);
        return;
        //exit(EXIT_FAILURE);
    }
    fprintf(table_log, "Finger table of : \n %d", ft->n);
    for(int i = 0; i != ft->m; i++){
        fprintf(table_log, "i : %d\t\tstart[i]-id : %d\t\tft[i]-id : %d\n", i, ft->start_ids[i], ft->entries[i]->id);
    }

}