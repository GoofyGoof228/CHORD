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

void int_to_str(int n, char* str){
    sprintf(str, "%d", n);
}
internal_message* look_up_to_reply(internal_message* m_in, peer_info* self){
    internal_message* out = (internal_message*) malloc(sizeof(internal_message));
    out->type = REPLY;
    out->node_id = self->next_id;
    out->node_ip = self->next_ip;
    out->node_port = self->next_port;
    //out->control = false;
    return out;
}

internal_message* create_look_up(external_message* m_ex, peer_info* self){
    internal_message * m_in = (internal_message*) malloc(sizeof(internal_message));
    m_in->type = LOOKUP;
    m_in->hash_id = get_hash_id(m_ex->data->key, m_ex->data->key_len);
    m_in->node_id = self->self_id;
    m_in->node_ip = self->self_ip;
    m_in->node_port = self->self_port;
    //m_in->control = false;
    return m_in;
}

internal_message* create_reply(peer_info* self, uint16_t hash_id){
    internal_message* reply = malloc(sizeof(internal_message));
    //reply->control = 0;
    reply->hash_id = hash_id;
    reply->node_id = self->next_id;
    reply->node_ip = self->next_ip;
    reply->node_port = self->next_port;
    reply->type = REPLY;
    return reply;
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