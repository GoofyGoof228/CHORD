//
// Created by Artem Sereda on 30.11.19.
//

#include "peer_help.h"
#include <stdio.h>
#include <arpa/inet.h>

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

external_message* get_saved_state(list* states, uint16_t hash_id){
    //iterate over list and find right saved message
    listIterator* it = listIteratorCreate(states);
    external_message* to_send = listIteratorGetCurrentElement(it);
    while( to_send != NULL){
        if(get_hash_id(to_send->data->key, to_send->data->key_len), hash_id){
            //right message found
            //listIteratorRemoveCurrent(it, NULL);
            break;
        }
        to_send = listIteratorGetNextElement(it);
    }
    free(it);
    return to_send;
}

external_message* pop_saved_state(list* states, uint16_t hash_id){
    //iterate over list and find right saved message
    listIterator* it = listIteratorCreate(states);
    external_message* to_send = listIteratorGetCurrentElement(it);
    while( to_send != NULL){
        if(get_hash_id(to_send->data->key, to_send->data->key_len), hash_id){
            //right message found
            //listIteratorRemoveCurrent(it, NULL);
            break;
        }
        to_send = listIteratorGetNextElement(it);
    }
    listIteratorRemoveCurrent(it, NULL);
    free(it);
    return to_send;
}
bool is_between(uint16_t hash, uint16_t prev, uint16_t now){
    if (prev > now) {
        return ((hash > prev) && (UINT16_MAX >= hash)) || (hash > 0 && hash <= now);
    }

    return hash > prev && hash <= now;
}