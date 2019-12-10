//
// Created by Artem Sereda on 22.11.19.
//

#include <sys/socket.h>
#include  <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "peer_netw.h"
#include "hash_table.h"
#include "peer_help.h"
#include "finger_table.h"
#include <netdb.h>

#define TEST


uint32_t get_ipv4_addr(char *name){
    int status;
    struct addrinfo hints;
    struct addrinfo *res, *p;  // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;     //  IPv4 only
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    if ((status = getaddrinfo(name, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    uint32_t result = 0;
    for(p = res;p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
        result = ipv4->sin_addr.s_addr;
        break;
    }
    freeaddrinfo(res);
    return ntohl(result);
}

int setup_listen_socket(uint16_t port_number, char * ip_str){
    char port[12];
    sprintf(port, "%d", port_number);
    struct addrinfo hints, *result, *p;
    int listen_sock = 0;
    int yes = 1;
    memset(&hints, 0, sizeof(hints));
    /**
     * when set to AF_INET doest recieve connections from localhost
     * but ew why ????
     * */
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip_str, port, &hints, &result) != 0) {
        fprintf(stderr, "Setup Listen Socket: getaddrinfo error : \n");
    }
    // Loop through results
    for (p = result; p != NULL; p = p->ai_next) {
        if ((listen_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Setup Listen Socket: at socket");
            continue;
        }

        if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("Setup Listen Socket: at setsockopt");
            return -1;
        }

        if (bind(listen_sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_sock);
            perror("Setup Listen Socket: at bind");
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if (p == NULL) {
        fprintf(stderr, "Setup Listen Socket: Bind failed");
        return -1;
    }
    if (listen(listen_sock, 10) == -1) {
        perror("Setup Listen Socket: at listen");
        return -1;
    }
    return listen_sock;
}

int connect_to_peer(uint32_t ip, uint16_t port){
    // Setup Structs
    struct in_addr sin_addr;
    sin_addr.s_addr = htonl(ip);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = sin_addr;

    // Get Socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        perror("connect_to_peer : socket\n");
        return -1;
    }
    // Connect
    if(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1){
        close(sock);
        perror("connect_to_peer : connect\n");
        return -1;
    }
    return sock;
}

external_message* do_hashtable_action(external_message *in, payload **hash){

    if(in->ack) {
        fprintf(stderr, "Error while doing Hashtable Action: Message is Ack\n");
        return NULL;
    }

    external_message* out = malloc(sizeof(external_message));
    out->ack = true;
    out->type = in->type;

    switch (in->type) {
        case GET: {
            out->data = h_get(hash, in->data->key, in->data->key_len);
            if(out->data == NULL){
                out->data = payload_deepcopy(in->data);
            }
            break;
        }
        case SET: {
            h_set_p(hash, in->data);
            out->data = create_empty_payload();

            break;
        }
        case DELETE: {
            h_del(hash, in->data->key, in->data->key_len);
            out->data = create_empty_payload();
            break;
        }
        default: {
            fprintf(stderr, "Error while Handeling Message: Message formated incorrectly\n");
            return NULL;
        }
    }
    return out;
}

int handle_internal_message(internal_message * m_in, peer_info * self, int socket, fd_set * master) {
    int peer_socket = -1;

    switch(m_in->type) {
        case LOOKUP: {
            // check if next one is resp peer
            if (is_between(m_in->hash_id, self->self_id, self->next_id)) {
                // send repl with info of next node
                internal_message *reply = new_internal_message(REPLY, m_in->hash_id, self->next_id, self->next_ip, self->next_port); //create_reply(self, m_in->hash_id);
                peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);

                if(send_internal_message(reply, peer_socket) == -1){
                    fprintf(stderr, " Sending Reply\n");
                    return -1;
                }
                close(peer_socket);
                free(reply);
            }
            else if (is_between(m_in->hash_id, self->previous_id, self->self_id)) {
                // send repl with info of next node
                internal_message *reply = new_internal_message(REPLY, m_in->hash_id, self->self_id, self->self_ip, self->self_port); //create_reply(self, m_in->hash_id);
                peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);

                if(send_internal_message(reply, peer_socket) == -1){
                    fprintf(stderr, " Sending Reply\n");
                    return -1;
                }
                close(peer_socket);
                free(reply);
            }
            else {
                // send to next peer
                peer_socket = connect_to_peer(self->next_ip, self->next_port);

                if(send_internal_message(m_in, peer_socket) == -1){
                    fprintf(stderr, " Sending Lookup onwards\n");
                    return -1;
                }
                close(peer_socket);
            }
            break;
        }
        case REPLY: {

            message *state = pop_saved_state(self->states, m_in->hash_id, INTERNAL_MES);

            if(state == NULL){
                fprintf(stderr, "Error: No Saved Message to match REPLY with Hash ID: %u", m_in->hash_id);
                return -1;
            }

            if(state->internal){
                if(state->int_msg->type == LOOKUP){
                    //TODO finish FT
                    recieve_reply_ft(state->int_msg, self);
                    free_message(state);
                }
                else {
                    fprintf(stderr, "Error: Internal Message of type %d in State", state->int_msg->type);
                    return -1;
                }
            }
            else {
                external_message* to_send = state->ext_msg;
                if (to_send == NULL) {
                    fprintf(stderr, "Error : trying to send NULL external message\n");
                    return -1;
                }
                peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
                send_external_message(to_send, peer_socket);
                FD_SET(peer_socket, master);

                // Save the client socket
                payload *p = ints_to_payload(peer_socket, to_send->socket_recieved_from);
                h_set_p(self->response_sockets_head, p);
                free_message(state);
                free_payload(p);
            }
            break;
        }
        case STABILIZE: {
            if (!self->initialised_previous) {
                // Set Prev for first time
                self->previous_id = m_in->node_id;
                self->previous_ip = m_in->node_ip;
                self->previous_port = m_in->node_port;
                self->initialised_previous = true;
            }
            else if (is_between(m_in->node_id, self->previous_id, self->self_id)) {
                // Update Prev
                self->previous_id = m_in->node_id;
                self->previous_ip = m_in->node_ip;
                self->previous_port = m_in->node_port;
                self->initialised_previous = true;
            }
            // send notify
            internal_message * out = new_internal_message(NOTIFY, 0, self->previous_id, self->previous_ip, self->previous_port);

            peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
            if(send_internal_message(out, peer_socket) == -1) {
                fprintf(stderr, " Sending Notify after Stabalize\n");
                return -1;
            }
            close(peer_socket);
            break;

        }
        case NOTIFY: {

            if(self->initialised_next) {
                // Check if I am Previous Node
                if(self->self_id != m_in->node_id) {
                    #ifdef TEST
                        //printf("before NOTIFY\n");
                        //print_peer_info_long(self);
                        printf("BN:%s\n", peer_info_to_str(self));
                    #endif
                    self->next_id = m_in->node_id;
                    self->next_ip = m_in->node_ip;
                    self->next_port = m_in->node_port;
                    #ifdef TEST
                        //printf("after NOTIFY\n");
                        //print_peer_info_long(self);
                        printf("AN:%s\n", peer_info_to_str(self));
                    #endif
                }
            }
            // Joining in progress:
            else{
                // Set Successor
                #ifdef TEST
                    //printf("before NOTIFY\n");
                    //print_peer_info_long(self);
                    printf("BJ:%s\n", peer_info_to_str(self));
                #endif
                self->next_id = m_in->node_id;
                self->next_ip = m_in->node_ip;
                self->next_port = m_in->node_port;
                self->initialised_next = true;
                #ifdef TEST
                    //printf("after NOTIFY\n");
                    //print_peer_info_long(self);
                    printf("AJ:%s\n", peer_info_to_str(self));
                #endif
            }
            break;
        }
        case JOIN: {
            // Do Join
            if((self->first_peer && !self->initialised_previous) || is_between(m_in->node_id, self->previous_id, self->self_id) ) {
                // Send Notify to Joining Node
                internal_message * out = new_internal_message(NOTIFY, 0, self->self_id, self->self_ip, self->self_port);
                peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
                if(send_internal_message(out, peer_socket) == -1) {
                    fprintf(stderr, " Sending Notify after Join\n");
                    return -1;
                }
                close(peer_socket);

                // Update Prev
                self->previous_id = m_in->node_id;
                self->previous_ip = m_in->node_ip;
                self->previous_port = m_in->node_port;
                self->initialised_previous = true;

                if(!self->initialised_next){

                    self->next_id = m_in->node_id;
                    self->next_ip = m_in->node_ip;
                    self->next_port = m_in->node_port;
                    self->initialised_next = true;
                }
            }
            // Send to next node
            else {
                peer_socket = connect_to_peer(self->next_ip, self->next_port);
                if(send_internal_message(m_in, peer_socket) == -1) {
                    fprintf(stderr, " Sending Join onwards\n");
                    return -1;
                }
                close(peer_socket);
            }
            break;
        }
        case F_ACK: {
            #ifdef TEST
                printf("I: %s\n", peer_info_to_str(self));
            #endif
            fprintf(stderr, "F_Ack: Not implemented\n");
            return -1;
            break;

        }
        case FINGER: {
            create_ft(self, socket);
            init_fill_ft(self);
            break;

        }
        default: {
            fprintf(stderr, "Unknown Internal Message Type\n");
            return -1;
        }
    }
    return 0;
}

int handle_external_message(external_message * m_ex, peer_info * self, int socket, fd_set * master){
    // Ack from Peer
    if(m_ex->ack == true){

        // TODO Clean up this ugly mess
        payload *p1 = ints_to_payload(socket, 0);
        payload *p2 = h_get(self->response_sockets_head, p1->key, p1->key_len);
        free_payload(p1);
        int client_sock = buf_to_int(p2->value, p2->val_len);
        h_del(self->response_sockets_head, p2->key, p2->key_len);
        free_payload(p2);

        // send answer back
        int res = send_external_message(m_ex, client_sock);
        FD_CLR(socket, master);
        close(socket);
        FD_CLR(client_sock, master);
        close(client_sock);
        return res;
    }
    // Normal Client Request
    else{
        uint16_t hash_id = get_hash_id(m_ex->data->key, m_ex->data->key_len);
        // Answer Directly
        if(is_between(hash_id, self->previous_id, self->self_id)){
            // do action on HT
            external_message* out = do_hashtable_action(m_ex, self->hash_head);
            if (out == NULL){
                fprintf(stderr, " Doing Hashtable Action \n");
                return -1;
            }
            // send reply(external, with ack)
            if (send_external_message(out, socket) == -1){
                fprintf(stderr, " Sending Direct Response to Client Request \n");
                return -1;
            }
            free_external_message(out);
            FD_CLR(socket, master);
            close(socket);
            return 0;
        }
        // Send Lookup
        else{
            // save the state
            message* to_save = create_wrapper(m_ex, EXTERNAL_MES);
            listPushBack(self->states, to_save);
            // create Lookup message
            int hash_value = get_hash_id(m_ex->data->key, m_ex->data->key_len);
            internal_message * out = new_internal_message(LOOKUP, hash_value, self->self_id, self->self_ip, self->self_port); //create_look_up(m_ex, self);
            int peer_socket = connect_to_peer(self->next_ip, self->next_port);
            if(send_internal_message(out, peer_socket) == -1){
                fprintf(stderr, " Sending initial Lookup for Client Request\n");
                return -1;
            }
            close(peer_socket);

            free(out);
            return 0;
        }
    }
}

int react_on_incoming_message(message* in, peer_info* self, int socket, fd_set* master){
    if(in == NULL){
        fprintf(stderr, "react_on_incoming_message: in == NULL\n");
        return -1;
    }
    if(self == NULL){
        fprintf(stderr, "react_on_incoming_message : self == NULL\n");
        return -1;
    }
    int res = -1;

    if(in->int_msg != NULL){

        close(socket);
        FD_CLR(socket, master);

        internal_message* m_in = in->int_msg;

        res = handle_internal_message(m_in, self, socket, master);
        free_message(in);
        return res;
    }
    else if(in->ext_msg != NULL){
        external_message* m_ex = in->ext_msg;
        #ifdef TEST
            printf("R: %12s\n", "Ext Msg");
            //print_external_message(m_ex);
        #endif
        res = handle_external_message(m_ex, self, socket, master);
    }
    else {
        fprintf(stderr, "react_on_incoming_message : both types are NULL\n");
        return -1;
    }
    return res;
}