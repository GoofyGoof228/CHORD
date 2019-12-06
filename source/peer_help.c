//
// Created by Artem Sereda on 30.11.19.
//

#include "peer_help.h"
#include "peer_netw.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#import <sys/socket.h>
#include <unistd.h>
#include "list.h"
#define NUM_BITS_IN_HASH 16
#ifndef TEST
#define TEST
#endif
#ifndef TEST
#define TEST
#endif
int setup_peer_info(peer_info * self, char *argv[], int argc){

    self->first_peer = false;
    self->initialised_previous = false;
    self->initialised_next = false;

    self->self_id = 0;

    self->previous_id = 0;
    self->previous_ip = 0;
    self->previous_port = 0;

    self->next_id = 0;
    self->next_ip = 0;
    self->next_port = 0;

    switch (argc) {
        case 5: {
            self->self_ip = get_ipv4_addr(argv[1]);
            self->self_port = atoi(argv[2]);
            self->self_id = atoi(argv[3]);
            // Set Join Node to Next
            self->join_ip = get_ipv4_addr(argv[4]);
            self->join_port = atoi(argv[5]);
            break;
        }
        case 4: {
            self->self_id = atoi(argv[3]);
        }
        case 3: {
            self->first_peer = true;
            self->self_ip = get_ipv4_addr(argv[1]);
            self->self_port = atoi(argv[2]);
            self->self_id = 0;
            break;
        }
        default: {
            fprintf(stderr,"Usage: ./peer IP Port [ID] [Peer-IP Peer-Port]\n");
            return -1;
        }
    }
    return 0;
}

message* get_saved_state(list* states, uint16_t hash_id, const int type){
    if(type == EXTERNAL_MES){
        listIterator* it = listIteratorCreate(states);
        message* help = listIteratorGetCurrentElement(it);
        external_message* to_send = help->ext_msg;
        while( to_send != NULL && it->current != NULL){
            if(get_hash_id(to_send->data->key, to_send->data->key_len) == hash_id){
                //right message found
                //listIteratorRemoveCurrent(it, NULL);
                break;
            }
            to_send = listIteratorGetNextElement(it);
        }
      //  listIteratorRemoveCurrent(it, NULL);
        free(it);
        return create_wrapper(to_send, EXTERNAL_MES);

    }else if(type == INTERNAL_MES){

        listIterator* it = listIteratorCreate(states);
        message* help = listIteratorGetCurrentElement(it);
        internal_message * to_send = help->int_msg;
        while( to_send != NULL){
            if(to_send->hash_id == hash_id){
                //right message found
                //listIteratorRemoveCurrent(it, NULL);
                break;
            }
            to_send = listIteratorGetNextElement(it);
        }
       // listIteratorRemoveCurrent(it, NULL);
        free(it);
        return create_wrapper(to_send, INTERNAL_MES);
    }
    return NULL;
}

message* pop_saved_state(list* states, uint16_t hash_id, const int type){
    //iterate over list and find right saved message
    //iterate over list and find right saved message
    if(type == EXTERNAL_MES){
        listIterator* it = listIteratorCreate(states);
        message* help = listIteratorGetCurrentElement(it);
        external_message* to_send = help->ext_msg;
        while( to_send != NULL && it->current != NULL){
            if(get_hash_id(to_send->data->key, to_send->data->key_len) == hash_id){
                //right message found
                //listIteratorRemoveCurrent(it, NULL);
                break;
            }
            to_send = listIteratorGetNextElement(it);
        }
        listIteratorRemoveCurrent(it, NULL);
        free(it);
        return create_wrapper(to_send, EXTERNAL_MES);

    }else if(type == INTERNAL_MES){

        listIterator* it = listIteratorCreate(states);
        message* help = listIteratorGetCurrentElement(it);
        internal_message * to_send = help->int_msg;
        while( to_send != NULL){
            if(to_send->hash_id == hash_id){
                //right message found
                //listIteratorRemoveCurrent(it, NULL);
                break;
            }
            to_send = listIteratorGetNextElement(it);
        }
        listIteratorRemoveCurrent(it, NULL);
        free(it);
        return create_wrapper(to_send, INTERNAL_MES);
    }
    return NULL;
}

bool is_between(uint16_t hash, uint16_t prev, uint16_t now){
    if (prev > now) {
        return ((hash > prev) && (UINT16_MAX >= hash)) || (hash > 0 && hash <= now);
    }

    return hash > prev && hash <= now;
}

void print_peer_info_short(peer_info* self){
    if(self == NULL){
        fprintf(stderr, "self == NULL");
        return;
    }
    printf("Peer info\n");
    printf("----------\n");
    printf("ID : %d\n", self->self_id);
    char buf[INET_ADDRSTRLEN];
    struct in_addr ip;
    ip.s_addr = self->self_ip;
    inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN);
    printf("IP : %s\n", buf);
    printf("PORT : %d\n", self->self_port);
    fflush(stdout);
}

void print_addr_info(uint16_t id, uint32_t ip_addr, uint32_t port){
    printf("ID : %d\n", id);
    char buf[INET_ADDRSTRLEN];
    struct in_addr ip;
    ip.s_addr = ip_addr;
    inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN);
    printf("IP : %s\n", buf);
    printf("Port : %d\n", port);
    fflush(stdout);
}

void print_peer_info_long(peer_info* self){
    if(self == NULL){
        fprintf(stderr, "self == NULL");
        return;
    }
    printf("Peer info\n");
    printf("----------\n");
    printf("ID : %d\n", self->self_id);

    char buf[INET_ADDRSTRLEN];
    struct in_addr ip;
    ip.s_addr = self->self_ip;
    inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN);
    printf("IP : %s\n", buf);

    printf("PORT : %d\n", self->self_port);

    printf("prev ID : %d\n", self->previous_id);

    char prev_buf[INET_ADDRSTRLEN];
    struct in_addr prev_ip;
    prev_ip.s_addr = self->previous_ip;
    inet_ntop(AF_INET, &prev_ip, prev_buf, INET_ADDRSTRLEN);
    printf("prev IP : %s\n", prev_buf);

    printf("prev PORT : %d\n", self->previous_port);

    printf("next ID : %d\n", self->next_id);

    char next_buf[INET_ADDRSTRLEN];
    struct in_addr next_ip;
    next_ip.s_addr = self->next_ip;
    inet_ntop(AF_INET, &next_ip, next_buf, INET_ADDRSTRLEN);
    printf("next IP : %s\n", next_buf);

    printf("next PORT : %d\n", self->next_port);
    printf("number of saved states : %d\n", listGetSize(self->states));
    fflush(stdout);

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