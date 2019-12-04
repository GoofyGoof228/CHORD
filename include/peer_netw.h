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
external_message* do_hashtable_action(external_message *in, payload **hash);

int setup_listen_socket(uint16_t port_number, char * ip_str);

int connect_to_peer(uint32_t ip, uint16_t port);

int handle_internal_message(internal_message * m_in, peer_info * self, int socket, fd_set * master);

int handle_external_message(external_message * m_ex, peer_info * self, int socket, fd_set * master);

/**
 * in - message that came
 *  self - peer info struct fron reciever
 *  socket - socket wich message was readen from (schould be KEEP_ALIVE, i guess)
 * */
int react_on_incoming_message(message* in, peer_info* self, int socket, fd_set* master);




#endif //BLOCK4_PEER_FUNC_H
