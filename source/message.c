//
// Created by Jonathan Burggraf on 21.11.19.
//

#include <stdio.h>
#include <sys/socket.h>
#include <stdint.h>
#include "message.h"
#ifndef TEST
#define TEST
#endif
bool is_internal(uint8_t byte){

    uint8_t control = byte & (uint8_t) 0x80;
    control = control >> 7u;

    if(control) {
        return true;
    }
    return false;
}
message* create_wrapper(void* m, const int type){
    if(type != EXTERNAL_MES && type != INTERNAL_MES){
        fprintf(stderr, "invalid type for wrapper!\n");
        return NULL;
    }
    message* new_m = malloc(sizeof(message));
    if(type == EXTERNAL_MES){
        new_m->ext_msg = (external_message*) m;
        new_m->int_msg = NULL;
    }
    if(type == INTERNAL_MES){
        new_m->int_msg = (internal_message*) m;
        new_m->ext_msg = NULL;
    }
    return new_m;
}
int recv_message(message *m, int sock){

    int bytes_recieved = 0;
    uint8_t *buf = calloc(EXTERNAL_HEADER_LEN, sizeof(uint8_t));
    // TODO: Non Blocking
    // Get first Byte
    bytes_recieved = recv(sock, buf, 1, MSG_WAITALL);
    if(bytes_recieved == -1){ // error
        perror("Failed to get message: first Byte");
        return -1;
    }
    // Get Internal Message
    if (is_internal (buf[0])) {

        m->internal = true;
        m->ext_msg = NULL;
        m->int_msg = calloc(1, sizeof(internal_message));

        buf = realloc(buf, INTERNAL_HEADER_LEN);
        if (buf == NULL) {
            fprintf(stderr, "Failed to get external_message: header - realloc error");
            return -1;
        }
        // TODO: Non Blocking
        // Getting Header
        bytes_recieved += recv(sock, buf + 1, INTERNAL_HEADER_LEN - 1, MSG_WAITALL);
        if(bytes_recieved == -1){ // error
            perror("Failed to get external_message: header - recv error");
            exit(EXIT_FAILURE);
        }
        if(bytes_recieved < INTERNAL_HEADER_LEN){
            fprintf(stderr," \nint\n Error while Recieving Header: Header not complete, only %d long\n", bytes_recieved);
            return -1;
        }
        if(decode_internal_header(buf, m->int_msg) == -1) {
            fprintf(stderr,"Error while Decodeing Header\n");
            return -1;
        }
    }
    // Get external message
    else {
        m->internal = false;
        m->int_msg = NULL;
        m->ext_msg = calloc(1, sizeof(external_message));

        m->ext_msg->data = calloc(1, sizeof(payload));
        m->ext_msg->ack = false;
        m->ext_msg->type = NOT_DEFINED;

        // Getting Header
        bytes_recieved += recv(sock, buf + 1, EXTERNAL_HEADER_LEN - 1, MSG_WAITALL);
        if(bytes_recieved == -1){ // error
            perror("Failed to get external_message: header - recv error");
            exit(EXIT_FAILURE);
        }
        if(bytes_recieved < EXTERNAL_HEADER_LEN){
            fprintf(stderr," \next: socket: %d\n Error while Recieving Header: Header not complete, only %d long\n", sock, bytes_recieved);
            return -1;
        }
        if(decode_external_header(buf, m->ext_msg) == -1) {
            fprintf(stderr,"Error while Decodeing Header\n");
            return -1;
        }

        // Getting Key
        bytes_recieved = 0;
        m->ext_msg->data->key = calloc(m->ext_msg->data->key_len, sizeof(uint8_t));
        if(m->ext_msg->data->key_len != 0){
            do {
                bytes_recieved += recv(sock, m->ext_msg->data->key + bytes_recieved, m->ext_msg->data->key_len, MSG_WAITALL);
                if(bytes_recieved == -1){ // error
                    perror("Failed to get external_message: key");
                    exit(EXIT_FAILURE);
                }
            } while (bytes_recieved < m->ext_msg->data->key_len);
        } else {
            m->ext_msg->data->key = NULL;
        }

        // Getting Value
        bytes_recieved = 0;
        m->ext_msg->data->value = calloc(m->ext_msg->data->val_len, sizeof(uint8_t));
        if (m->ext_msg->data->val_len != 0) {
            do {
                bytes_recieved += recv(sock, m->ext_msg->data->value + bytes_recieved, m->ext_msg->data->val_len, MSG_WAITALL);

                if (bytes_recieved == -1) { // error
                    perror("Failed to get external_message: value");
                    exit(EXIT_FAILURE);
                }
            } while (bytes_recieved < m->ext_msg->data->val_len);
        } else {
            m->ext_msg->data->value = NULL;
        }
        m->ext_msg->socket_recieved_from = sock;
    }
    free(buf);
    return 0;
}

void free_message(message* old){
    if(old->ext_msg != NULL)free_external_message(old->ext_msg);
    else if (old->int_msg != NULL)free(old->int_msg);
    free(old);
}

void print_message(message* m){
    if(m->ext_msg == NULL && m->int_msg != NULL){
        print_internal_message(m->int_msg);
        return;
    }else if(m->ext_msg != NULL && m->int_msg == NULL){
        print_external_message(m->ext_msg);
        return;
    }
    fprintf(stderr, "NULL message !\n");
}