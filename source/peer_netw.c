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
#define SOCKET int
#define TEST
#define LOG_SN 0
//#define FT_M
//#define DG_FT
#define SOCK_OUT
void close_socket(SOCKET socket){
    if(socket != -1){
        close(socket);
        socket = -1;
    }else{
    #ifdef SOCK_OUT
       fprintf(stderr, "Trying to close socket twice\n");
    #endif
    }

}
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
            close_socket(listen_sock);
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

// TO DO: Cleanup
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
        close_socket(sock);
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

int handle_internal_message(internal_message * m_in, peer_info * self, SOCKET socket, fd_set * master) {
    int peer_socket = -1;

    switch(m_in->type) {
        case LOOKUP: {
            // check if next one is resp peer

                if(self->ft != NULL){
                    if(((finger_table*)self->ft)->filled){
                        #ifdef TEST
                        printf("FT used after lookup for reply\n");
                        #endif
                        ft_entry* result = find_corresponding_peer(self->ft, m_in->hash_id);
                        if(result != NULL){
                            //peer found - send reply
                        #ifdef DG_FT
                        printf("Found entry\n");
                        print_entry(result);
                        #endif
                            internal_message *reply = new_internal_message(REPLY, m_in->hash_id, result->id, result->ip, result->port);
                            peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
                            if(send_internal_message(reply, peer_socket) == -1){
                                fprintf(stderr, " Sending Reply\n");
                                return -1;
                            }
                            close_socket(peer_socket);
                            free(reply);
                            free(result);
                            return 0;
                        }else{
                            //send lookup to last known peer
                            result = get_last_entry(self->ft);
                            #ifdef DG_FT
                            printf("forwarding lookup to last entry\n");
                            print_entry(result);
                            #endif
                            //TO DO check not to senbd it urself
                            if(result->id == self->self_id){
                                internal_message *reply = new_internal_message(REPLY, m_in->hash_id, self->self_id, self->self_ip, self->self_port);
                                peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
                                if(send_internal_message(reply, peer_socket) == -1){
                                    fprintf(stderr, " Sending Reply\n");
                                    return -1;
                                }
                                close_socket(peer_socket);
                                free(reply);
                                free(result);
                                return 0;
                            }
                            peer_socket = connect_to_peer(result->ip, result->port);
                            send_internal_message(m_in, peer_socket);
                            close_socket(peer_socket);
                            free(result);
                            return 0;
                        }
                    }

                }else{
                    //TO DO do it in old manner
                    if (is_between(m_in->hash_id, self->self_id, self->next_id)) {
                        // send repl with info of next node
                        internal_message *reply = new_internal_message(REPLY, m_in->hash_id, self->next_id, self->next_ip, self->next_port);
                        peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);

                    if(send_internal_message(reply, peer_socket) == -1){
                        fprintf(stderr, " Sending Reply\n");
                        return -1;
                    }
                    close_socket(peer_socket);
                    free(reply);
                    }else if(is_between(m_in->hash_id, self->previous_id, self->self_id)){
                        //TO DO also check me, for FT case

                        // send repl with info of me
                        internal_message *reply = new_internal_message(REPLY, m_in->hash_id, self->previous_id, self->self_ip, self->self_port);
                        peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);

                        if(send_internal_message(reply, peer_socket) == -1){
                            fprintf(stderr, " Sending Reply\n");
                            return -1;
                        }
                        close_socket(peer_socket);
                        free(reply);
                    }else {
                        // send to next peer
                        peer_socket = connect_to_peer(self->next_ip, self->next_port);

                        if(send_internal_message(m_in, peer_socket) == -1){
                            fprintf(stderr, " Sending Lookup onwards\n");
                            return -1;
                        }
                        close_socket(peer_socket);
                    }
                    return 0;
                }


        }
        case REPLY: {

            //TO DO kaka
            internal_message *state = pop_saved_state_int(self->internal_states, m_in->hash_id);
            if(state != NULL){
                    recieve_reply_ft(m_in, self);
                    free(state);
                    close_socket(socket);
                    return 0;
            }
            /*if(state == NULL){
                fprintf(stderr, "Error: No Saved Message to match REPLY with Hash ID: %u", m_in->hash_id);
                return -1;
            }*/
            external_message* to_send = pop_saved_state_ext(self->external_states, m_in->hash_id);
                if (to_send == NULL) {
                    fprintf(stderr, "Error : trying to send NULL external message\n");
                    return -1;
                }
                peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
                send_external_message(to_send, peer_socket);
                // Save the peer socket
                FD_SET(peer_socket, master);
                payload *p = ints_to_payload(peer_socket, to_send->socket_recieved_from);
                h_set_p(self->response_sockets_head, p);
                free_external_message(to_send);
                free_payload(p);
            return 0;
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
            close_socket(peer_socket);
            return 0;

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
                close_socket(peer_socket);

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
                close_socket(peer_socket);
            }
            break;
        }
        case F_ACK: {
        #ifdef FT_M
        printf("Finger table was succesfully built by peer with id %d\n", m_in->node_id);
        #endif
        #ifdef FT_KEEP_ALIVE
            //FD_CLR(socket, master);
        #endif
            close_socket(socket);
            return 0;

        }
        case FINGER: {
        #ifdef DG_FT
         printf("saved FINGER message\n");
         print_internal_message(m_in);
        #endif
        #ifndef FT_KEEP_ALIVE
            internal_message* to_save = copy_int_message(m_in);
            listPushBack(self->internal_states, to_save);
            if(self->ft != NULL) free_ft(self->ft);
            create_ft(self, socket);
            init_fill_ft(self);
            //??? close_socket(socket);
        #endif
        #ifdef FT_KEEP_ALIVE
            if(self->ft != NULL) free_ft(self->ft);
            create_ft(self, socket);
            init_fill_ft(self);
        #endif
            return 0;

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

        // TO DO Clean up this ugly mess
        payload *p1 = ints_to_payload(socket, 0);
        payload *p2 = h_get(self->response_sockets_head, p1->key, p1->key_len);
        free_payload(p1);
        int client_sock = buf_to_int(p2->value, p2->val_len);
        h_del(self->response_sockets_head, p2->key, p2->key_len);
        free_payload(p2);

        // send answer back
        int res = send_external_message(m_ex, client_sock);
        FD_CLR(socket, master);
        close_socket(socket);
        FD_CLR(client_sock, master);
        close_socket(client_sock);
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
            close_socket(socket);
            return 0;
        }
        // Send Lookup
        else{
            // save the state
            listPushBack(self->external_states, m_ex);
            int hash_value = get_hash_id(m_ex->data->key, m_ex->data->key_len);


            if(self->ft != NULL){
                if(((finger_table*)self->ft)->filled){
                    #ifdef TEST
                    printf("FT used for lookup\n");
                    #endif
                    ft_entry* result = find_corresponding_peer(self->ft, hash_value);
                    #ifdef TEST
                    printf("FT: result\n");
                    print_entry(result);
                    #endif
                    if(result == NULL){
                        //TO DO send lookup on next
                        #ifdef TEST
                        printf("lookup sended to last entry\n");
                        #endif
                        internal_message * out = new_internal_message(LOOKUP, hash_value, self->self_id, self->self_ip, self->self_port);
                        //create_look_up(m_ex, self);
                        result = get_last_entry(self->ft);
                        int peer_socket = connect_to_peer(result->ip, result->port);
                        if(send_internal_message(out, peer_socket) == -1){
                            fprintf(stderr, " Sending initial Lookup for Client Request\n");
                            return -1;
                        }
                        close_socket(peer_socket);
                        free(result);
                        free(out);
                        return 0;
                    }else{
                        //TO DO send action (extr)
                        #ifdef TEST
                        printf("external message will be forwarded\n");
                        #endif
                        int peer_socket = connect_to_peer(result->ip, result->port);
                        send_external_message(m_ex, peer_socket);
                        FD_SET(peer_socket, master);

                        // Save the client socket
                        payload *p = ints_to_payload(peer_socket, m_ex->socket_recieved_from);
                        h_set_p(self->response_sockets_head, p);
                        //free_message();
                        free_payload(p);
                    }
                }

            }
            else{
                //TO DO do it in old manner
                internal_message * out = new_internal_message(LOOKUP, hash_value, self->self_id, self->self_ip, self->self_port); //create_look_up(m_ex, self);
                int peer_socket = connect_to_peer(self->next_ip, self->next_port);
                if(send_internal_message(out, peer_socket) == -1){
                    fprintf(stderr, " Sending initial Lookup for Client Request\n");
                    return -1;
                }
                close_socket(peer_socket);

                free(out);
                return 0;
            }



        }
    }
    return 0;
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

        #ifdef FT_KEEP_ALIVE
        if(in->int_msg->type != FINGER){
            close_socket(socket);
            FD_CLR(socket, master);
        }
        #endif
        #ifndef FT_KEEP_ALIVE
        close_socket(socket);
        FD_CLR(socket, master);
        #endif
        internal_message* m_in = in->int_msg;

        res = handle_internal_message(m_in, self, socket, master);
        free_message(in);
        return res;
    }
    else if(in->ext_msg != NULL){
        external_message* m_ex = in->ext_msg;
        #ifdef TEST
            printf("R: %12s id :%d\n", "Ext Msg", get_hash_id(m_ex->data->key, m_ex->data->key_len));
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