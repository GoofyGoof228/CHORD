//
// Created by Jonathan Burggraf on 22.11.19.
//

#include "external_message.h"

#include <stdio.h>
#include <sys/socket.h>
#define TEST
int encode_external_message(uint8_t *buf, external_message *msg){
    msg->ack ? (buf[0] = (uint8_t) 0x08) : (buf[0] = (uint8_t) 0x00);

    // encode action
    switch (msg->type) {
        case GET : {
            buf[0] = buf[0] | (uint8_t) 0x04;
            break;
        }
        case SET : {
            buf[0] = buf[0] | (uint8_t) 0x02;
            break;
        }
        case DELETE : {
            buf[0] = buf[0] | (uint8_t) 0x01;
            break;
        }
        default : {
            return - 1;
        }
    }

    // encode key length
    uint16_t msb = (uint16_t) (msg->data->key_len & (uint16_t) 0xff00) >> (uint16_t) 8;
    uint16_t lsb = msg->data->key_len & (uint16_t) 0x00ff;
    buf[1] = (uint8_t) msb;
    buf[2] = (uint8_t) lsb;

    // encode value length
    uint32_t b1 = (uint32_t) (msg->data->val_len & (uint32_t) 0xff000000) >> (uint32_t) 24;
    uint32_t b2 = (uint32_t) (msg->data->val_len & (uint32_t) 0x00ff0000) >> (uint32_t) 16;;
    uint32_t b3 = (uint32_t) (msg->data->val_len & (uint32_t) 0x0000ff00) >> (uint32_t) 8;
    uint32_t b4 = (uint32_t) (msg->data->val_len & (uint32_t) 0x000000ff);

    buf[3] = (uint8_t) b1;
    buf[4] = (uint8_t) b2;
    buf[5] = (uint8_t) b3;
    buf[6] = (uint8_t) b4;

    if(msg->data != NULL){
        // encode key
        if(msg->data->key_len){
            memcpy(&buf[7], msg->data->key, msg->data->key_len);
        }
        // encode value
        if(msg->data->val_len){
            memcpy(&buf[7 + msg->data->key_len], msg->data->value, msg->data->val_len);
        }
    }

    //buf = b;
    return EXTERNAL_HEADER_LEN + msg->data->key_len + msg->data->val_len;
}

int decode_external_header(uint8_t *buf, external_message *msg){
    //determine ack
    uint8_t ack = buf[0] & (uint8_t) 0x08; // 0x08 = 0000 1000
    ack = ack >> (uint8_t) 3;
    msg->ack = (bool) ack;

    int8_t action = buf[0] & (uint8_t) 0x07; // 0x08 = 0000 0111
    switch (action){
        case 1: {
            msg->type = DELETE;
            break;
        }
        case 2: {
            msg->type = SET;
            break;
        }
        case 4: {
            msg->type = GET;
            break;
        }
        default: {
            msg->type = NOT_DEFINED;
            return -1;
        }
    }

    // determine key length
    uint16_t msb = buf[1] << (uint8_t) 8;
    uint16_t lsb = buf[2];
    msg->data->key_len = msb | lsb;

    // determine value length
    uint32_t b1 = buf[3] << (uint8_t) 24;
    uint32_t b2 = buf[4] << (uint8_t) 16;
    uint32_t b3 = buf[5] << (uint8_t) 8;
    uint32_t b4 = buf[6];
    msg->data->val_len = b1 | b2 | b3 | b4;

    // Point to external_message
    return 0;
}

int send_external_message(external_message *m, int client_sock){
#ifdef TEST
    //printf("sending\n");
    //print_external_message(m);
    //fflush(stdout);
#endif
    int key_len = 0;
    int val_len = 0;

    if(m->data != NULL){

        key_len = m->data->key_len;
        val_len = m->data->val_len;

    }

    uint8_t *buf = calloc(EXTERNAL_HEADER_LEN + key_len + val_len, sizeof(uint8_t));

    int bytes_sent = 0;

    int buf_len = encode_external_message(buf, m);

    if(buf_len == -1){
        fprintf(stderr, "Error while Encodeing Message\n");
        return -1;
    }

    do {

        bytes_sent += send(client_sock, buf + bytes_sent, buf_len, 0);

        if(bytes_sent == -1){

            perror("Error while Sending Message: send() error");
            return -1;
        }


    } while (bytes_sent < buf_len);
    free(buf);
    #ifdef TEST
        //printf("\nSent:\n");
        //print_external_message(m);
        //fflush(stdout);
    #endif

    return 0;
}

void free_external_message(external_message* m){
    if(m == NULL){
        fprintf(stderr, "NUllpointer exception at : free messsage : m == NULL");
        return;
    }
    if(m->data == NULL){
        fprintf(stderr, "NUllpointer exception at : free messsage : data == NULL");
        free(m);
        return;
    }
    free_payload(m->data);
    free(m);
}

void print_external_message(external_message *m) {
    if (m == NULL) {
        printf("( Empty Message )");
        return;
    }
    switch (m->type) {
        case GET: {
            printf("| Type: GET ");
            break;
        }
        case SET: {
            printf("| Type: SET ");
            break;
        }
        case DELETE: {
            printf("| Type: DELETE ");
            break;
        }
        default: {
            printf("| Type: NOT_DEFINED ");
            break;
        }
    }
    printf("| Ack: %d\n", m->ack);
    if (m->data == NULL)return;
    printf("| Key Length: %d\n", m->data->key_len);
    printf("| Key: ");
    for (uint32_t i = 0; i < m->data->key_len; i++) printf("%c", m->data->key[i]);
    printf("\n");
    printf("| Value Length: %d\n", m->data->val_len);
    if (m->data->value == NULL)return;
    printf("| Value: ");
    if (m->data->val_len < 20) {
        for (uint32_t i = 0; i < m->data->val_len; i++) printf(" %d", m->data->value[i]);
    } else {
        printf(" [Value too long]");
    }
    printf(" )\n");
    fflush(stdout);

}

uint16_t get_hash_id(uint8_t* key, uint16_t key_len){
    uint16_t id = 0;
    if(key_len >= 2){
        id = key[0] << 8u;
        id += key[1];
    }else if(key_len == 1){
        id = key[0];
    }else if(key_len == 0)return 0;
    return id;
}