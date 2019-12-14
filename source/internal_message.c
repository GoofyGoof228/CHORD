//
// Created by Jonathan Burggraf on 21.11.19.
//


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "internal_message.h"
#define TEST
#define LOG_SN 0
int encode_internal_message (uint8_t *buf, internal_message *m) {

    // encode action
    switch (m->type) {
        case FINGER : {
            buf[0] = buf[0] | (uint8_t) 0xc0;
            break;
        }
        case F_ACK : {
            buf[0] = buf[0] | (uint8_t) 0xa0;
            break;
        }
        case JOIN : {
            buf[0] = buf[0] | (uint8_t) 0x90;
            break;
        }
        case NOTIFY : {
            buf[0] = buf[0] | (uint8_t) 0x88;
            break;
        }
        case STABILIZE : {
            buf[0] = buf[0] | (uint8_t) 0x84;
            break;
        }
        case REPLY : {
            buf[0] = buf[0] | (uint8_t) 0x82;
            break;
        }
        case LOOKUP : {
            buf[0] = buf[0] | (uint8_t) 0x81;
            break;
        }
        default : {
            return - 1;
        }
    }

    // encode Hash ID
    uint16_t msb = (uint16_t) (m->hash_id & (uint16_t) 0xff00) >> (uint16_t) 8;
    uint16_t lsb = m->hash_id & (uint16_t) 0x00ff;
    buf[1] = (uint8_t) msb;
    buf[2] = (uint8_t) lsb;

    // encode Node ID
    msb = (uint16_t) (m->node_id & (uint16_t) 0xff00) >> (uint16_t) 8;
    lsb = m->node_id & (uint16_t) 0x00ff;
    buf[3] = (uint8_t) msb;
    buf[4] = (uint8_t) lsb;

    // encode Node IP
    uint32_t b1 = (uint32_t) (m->node_ip & (uint32_t) 0xff000000) >> (uint32_t) 24;
    uint32_t b2 = (uint32_t) (m->node_ip & (uint32_t) 0x00ff0000) >> (uint32_t) 16;;
    uint32_t b3 = (uint32_t) (m->node_ip & (uint32_t) 0x0000ff00) >> (uint32_t) 8;
    uint32_t b4 = (uint32_t) (m->node_ip & (uint32_t) 0x000000ff);
    buf[5] = (uint8_t) b1;
    buf[6] = (uint8_t) b2;
    buf[7] = (uint8_t) b3;
    buf[8] = (uint8_t) b4;

    // encode Node Port
    msb = (uint16_t) (m->node_port & (uint16_t) 0xff00) >> (uint16_t) 8;
    lsb = m->node_port & (uint16_t) 0x00ff;
    buf[9] = (uint8_t) msb;
    buf[10] = (uint8_t) lsb;

    return INTERNAL_HEADER_LEN;
}

int decode_internal_header (uint8_t *buf, internal_message *m) {
    // Type
    int8_t action = buf[0] & (uint8_t) 0x7f; // 0x/f = 0111 1111
    switch (action){
        case 1: {
            m->type = LOOKUP;
            break;
        }
        case 2: {
            m->type = REPLY;
            break;
        }
        case 4: {
            m->type = STABILIZE;
            break;
        }
        case 8: {
            m->type = NOTIFY;
            break;
        }
        case 16: {
            m->type = JOIN;
            break;
        }
        case 32: {
            m->type = F_ACK;
            break;
        }
        case 64: {
            m->type = FINGER;
            break;
        }
        default: {
            return -1;
        }
    }
    // Hash ID
    uint16_t msb = buf[1] << (uint8_t) 8;
    uint16_t lsb = buf[2];
    m->hash_id = msb | lsb;
    msb = 0;
    lsb = 0;

    // Node ID
    msb = buf[3] << (uint8_t) 8;
    lsb = buf[4];
    m->node_id = msb | lsb;
    msb = 0;
    lsb = 0;

    // Node IP
    uint32_t b1 = buf[5] << (uint8_t) 24;
    uint32_t b2 = buf[6] << (uint8_t) 16;
    uint32_t b3 = buf[7] << (uint8_t) 8;
    uint32_t b4 = buf[8];
    m->node_ip = b1 | b2 | b3 | b4;

    // Node Port
    msb = buf[9] << (uint8_t) 8;
    lsb = buf[10];
    m->node_port = msb | lsb;

    return 0;
}

int send_internal_message(internal_message *m, int sock){
    #ifdef TEST
    //if(m->type != STABILIZE){
        //printf("Sent:");
        //print_internal_message(m);
    //}
    if(LOG_SN){
        printf("S: %s\n", internal_message_to_str(m));
    } else if (m->type != NOTIFY && m->type != STABILIZE && m->type != JOIN){
        printf("S: %s\n", internal_message_to_str(m));
    }


    #endif
    uint8_t *buf = calloc(INTERNAL_HEADER_LEN, sizeof(uint8_t));

    int bytes_sent = 0;

    int buf_len = encode_internal_message(buf, m);

    if(buf_len == -1){
        fprintf(stderr, "Error while Encodeing Internal Message\n");
        return -1;
    }
    do {
        bytes_sent += send(sock, buf + bytes_sent, buf_len, 0);

        if(bytes_sent == -1){

            perror("Error while Sending Internal Message: send() error");
            return -1;
        }
    } while (bytes_sent < buf_len);

    free(buf);

    return 0;
}

internal_message * new_internal_message (internal_action type, uint16_t hash_id, uint16_t id, uint32_t ip, uint32_t port) {
    internal_message* m = calloc(1, sizeof(internal_message));
    if(m == NULL) {
        return NULL;
    }
    m->type = type;
    m->hash_id = hash_id;
    m->node_id = id;
    m->node_ip = ip;
    m->node_port = port;
    return m;
}



void print_internal_message(internal_message* m){
    if(m == NULL){
        fprintf(stderr, "void print_internal_message : m == NULL");
        return;
    }
    printf("internal message\n");
    printf("----------------\n");
    printf("type : ");
    switch (m->type){
        case LOOKUP: printf("LOOKUP\n"); break;
        case REPLY: printf("REPLY\n"); break;
        case JOIN: printf("JOIN\n"); break;
        case STABILIZE: printf("STABILIZE\n"); break;
        case NOTIFY: printf("NOTIFY\n"); break;
        case FINGER: printf("FINGER\n"); break;
        case F_ACK: printf("F_ACK\n"); break;
        default: printf("Unknown\n");
    }

    printf("hash id : %d\n", m->hash_id);
    printf("node id : %d\n", m->node_id);
    char buf[INET_ADDRSTRLEN];
    struct in_addr ip;
    ip.s_addr = htonl(m->node_ip);    inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN);
    printf("node ip : %s\n", buf);
    printf("node port : %d\n", m->node_port);
    fflush(stdout);

}


char * internal_message_to_str(internal_message *m){
    char buf[INET_ADDRSTRLEN];
    struct in_addr ip;
    ip.s_addr = htonl(m->node_ip);
    inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN);
    char *res = calloc(100, sizeof(char));
    switch (m->type){
        case LOOKUP: snprintf(res, 100* sizeof(char), "%12s%10u @ %u:%s\tH: %10u","LOOKUP", m->node_id, m->node_port, buf, m->hash_id); break;
        case REPLY: snprintf(res, 100* sizeof(char),"%12s%10u @ %u:%s\tH: %10u", "REPLY", m->node_id, m->node_port, buf, m->hash_id); break;
        case JOIN: snprintf(res, 100* sizeof(char),"%12s%10u @ %u:%s","JOIN", m->node_id, m->node_port, buf); break;
        case STABILIZE: snprintf(res, 100* sizeof(char),"%12s%10u @ %u:%s","STABILIZE", m->node_id, m->node_port, buf); break;
        case NOTIFY: snprintf(res, 100* sizeof(char),"%12s%10u @ %u:%s","NOTIFY", m->node_id, m->node_port, buf); break;
        case FINGER: snprintf(res, 100* sizeof(char),"%12s%10u @ %u:%s","FINGER", m->node_id, m->node_port, buf); break;
        case F_ACK: snprintf(res, 100* sizeof(char),"%12s%10u @ %u:%s","F_ACK", m->node_id, m->node_port, buf); break;
        default: snprintf(res, 100* sizeof(char),"Unknown");
    }

    return res;
}