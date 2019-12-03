//
// Created by Jonathan Burggraf on 21.11.19.
//


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "internal_message.h"

void print_internal_message(internal_message* m){
    if(m == NULL){
        fprintf(stderr, "void print_internal_message : m == NULL");
        return;
    }
    printf("internal message\n");
    printf("----------------\n");
    printf("control : %s\n", (m->control == true) ? "true" : " false");
    printf("type : %s\n", (m->type == LOOKUP)? "LOOKUP" : "REPLY");
    printf("hash id : %d\n", m->hash_id);
    printf("node id : %d\n", m->node_id);
    char buf[INET_ADDRSTRLEN];
    struct in_addr ip;
    ip.s_addr = m->node_ip;
    inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN);
    printf("node ip : %s\n", buf);
    printf("node port : %d\n", m->node_port);
    fflush(stdout);
}

int encode_internal_message (uint8_t *buf, internal_message *m) {

    // encode action
    switch (m->type) {
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
    int8_t action = buf[0] & (uint8_t) 0x03; // 0x03 = 0000 0011
    switch (action){
        case 1: {
            m->type = LOOKUP;
            break;
        }
        case 2: {
            m->type = REPLY;
            break;
        }
        default: {
            //m->tyEDpe = NOT_DEFIN;
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
    printf("\nSending");
#endif
    uint8_t *buf = calloc(INTERNAL_HEADER_LEN, sizeof(uint8_t));

    int bytes_sent = 0;

    int buf_len = encode_internal_message(buf, m);

    if(buf_len == -1){
        fprintf(stderr, "Error while Encodeing Internal Message\n");
        return -1;
    }
#ifdef TEST
    printf(" . ");
#endif
    do {
        bytes_sent += send(sock, buf + bytes_sent, buf_len, 0);

        if(bytes_sent == -1){

            perror("Error while Sending Internal Message: send() error");
            return -1;
        }
#ifdef TEST
        printf(" . ");
#endif

    } while (bytes_sent < buf_len);
#ifdef TEST
    printf(" . ");
    printf("Sent:\n");
    print_internal_message(m);
#endif
    free(buf);


    return 0;
}

