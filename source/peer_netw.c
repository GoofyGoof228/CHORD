//
// Created by Artem Sereda on 22.11.19.
//

#include <sys/socket.h>
#include  <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "peer_netw.h"
#include "hash_table.h"

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
    return result;
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
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip_str, port, &hints, &result) != 0) {
        fprintf(stderr, "Server: getaddrinfo error : \n");
    }
    // Loop through results
    for (p = result; p != NULL; p = p->ai_next) {
        if ((listen_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Server: at socket");
            continue;
        }

        if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("Server: at setsockopt");
            return -1;
        }

        if (bind(listen_sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_sock);
            perror("Server: at bind");
            continue;
        }
        break;
    }
    freeaddrinfo(result);
    if (p == NULL) {
        fprintf(stderr, "Server: Bind failed");
        return -1;
    }
    if (listen(listen_sock, 10) == -1) {
        perror("Server: at listen");
        return -1;
    }

    return listen_sock;
}

external_message* get_ext_msg_response(external_message *in, payload **hash){

    if(in->ack) {
        fprintf(stderr, "Error while Handeling Message: Message is Ack\n");
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

int connect_to_peer(uint32_t ip, uint16_t port){
    struct addrinfo client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.ai_family = AF_UNSPEC;
    client_addr.ai_socktype = SOCK_STREAM;
    client_addr.ai_flags = AI_PASSIVE; // ToDO Do we need ai_flags?
    struct addrinfo *server_adrr;
    char str_port[12];
    sprintf(str_port, "%d", port);
    char *str_ip4;
    struct in_addr ip_addr;
    ip_addr.s_addr = ip;
    str_ip4 = inet_ntoa(ip_addr);

    int status = getaddrinfo(str_ip4 ,str_port, &client_addr, &server_adrr);
    if (status != 0){
        const char* err = gai_strerror(status);
        fprintf(stderr, "%s", err);
        exit(EXIT_FAILURE);
    }
    int sock = 0;
    for (struct addrinfo *i = server_adrr; i != NULL; i = i->ai_next){
        sock = socket(i->ai_family, i->ai_socktype, i->ai_protocol);

        if(sock == -1){
            perror("Client : socket\n");
            continue;
        }

        if(connect(sock, server_adrr->ai_addr, server_adrr->ai_addrlen) == -1){
            close(sock);
            perror("Client : connect\n");
            continue;
        }
        break;
    }
    //setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, 1);
    freeaddrinfo(server_adrr);
    return sock;
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

    if(in->int_msg != NULL){
        internal_message* m_in = in->int_msg;
        #ifdef TEST
            printf("\nRecived:\n");
            print_internal_message(m_in);
        #endif
        print_internal_message(m_in);
        if(m_in->type == LOOKUP){
            // check if next one is resp peer
            if(is_between(m_in->hash_id, self->self_id, self->next_id)){
                // send repl with info of next node
                internal_message* reply = new_internal_message(REPLY, m_in->hash_id, self->next_id, self->next_ip, self->next_port); //create_reply(self, m_in->hash_id);
                int peer_socket = connect_to_peer(m_in->node_ip, m_in->node_port);
                send_internal_message(reply, peer_socket);
                close(peer_socket);
                free(reply);
                close(socket);
                FD_CLR(socket, master);
                free_message(in);
                return 0;
            }else{
                // send to next peer
                int next_peer_socket = connect_to_peer(self->next_ip, self->next_port);
                send_internal_message(m_in, next_peer_socket);
                close(next_peer_socket);
                close(socket);
                FD_CLR(socket, master);
                free_message(in);
                return 0;
            }
        }else if(m_in->type == REPLY){
           external_message* to_send = get_saved_state(self->states, m_in->hash_id);
            if(to_send == NULL){
                fprintf(stderr, "Eror : trying to send NULL external message\n");
                return -1;
            }

            close(socket);
            FD_CLR(socket, master);

            int peer_sock = connect_to_peer(m_in->node_ip, m_in->node_port);
            send_external_message(to_send, peer_sock);
            FD_SET(peer_sock, master);

            // Save the client socket
            payload * p = ints_to_payload(peer_sock, to_send->socket_recieved_from);
            h_set_p(self->response_sockets_head, p);
            free_external_message(to_send);
            free_payload(p);
            free_message(in);
            return 0;
        }
    }
    else if(in->ext_msg != NULL){
        external_message* m_ex = in->ext_msg;
        #ifdef TEST
            printf("\nRecived:\n");
            print_external_message(m_ex);
        #endif
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
            free_message(in);
            return res;
        }
        // Normal Client Request
        else{
            uint16_t hash_id = get_hash_id(m_ex->data->key, m_ex->data->key_len);

            // Answer Directly
            if(is_between(hash_id, self->previous_id, self->self_id)){

                // do action on HT, send reply(external, with ack)
                external_message* out = get_ext_msg_response(m_ex, self->hash_head);

                send_external_message(out, socket);
                free_external_message(out);
                free_message(in);
                FD_CLR(socket, master);
                close(socket);
                return 0;
            }
            // Send Lookup
            else{
                // save the state
                listPushFront(self->states, m_ex);
                // create Lookup message
                int hash_value = get_hash_id(m_ex->data->key, m_ex->data->key_len);
                internal_message * out = new_internal_message(LOOKUP, hash_value, self->self_id, self->self_ip, self->self_port); //create_look_up(m_ex, self);
                // send look up
                int next_peer_socket = connect_to_peer(self->next_ip, self->next_port);
                send_internal_message(out, next_peer_socket);
                close(next_peer_socket);
                free(out);
                return 0;
            }
        }
    }
    else {
        fprintf(stderr, "react_on_incoming_message : both types are NULL\n");
        return -1;
    }
    return 0;
}