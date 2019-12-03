//
// Created by Artem Sereda on 22.11.19.
//

#ifndef BLOCK4_PEER_FUNC_H
#define BLOCK4_PEER_FUNC_H

#include <netdb.h>
#include <stdio.h>
#include "message.h"
#include "list.h"
#include "peer_help.h"
//#define TEST

uint32_t get_ipv4_addr(char *name);

/**
 * in - filled struct
 * out - staticly declared pointer
 * all inner alloc will be done in func
 * must be freed afterwards
 * */
external_message* get_ext_msg_response(external_message *in, payload **hash);

/**
 * in - message that came
 *  self - peer info struct fron reciever
 *  socket - socket wich message was readen from (schould be KEEP_ALIVE, i guess)
 * */
int react_on_incoming_message(message* in, peer_info* self, int socket, fd_set* master);

int connect_to_peer(uint32_t ip, uint16_t port);


#endif //BLOCK4_PEER_FUNC_H
