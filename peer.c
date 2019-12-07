#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "hash_table.h"
#include "peer_netw.h"
#include "peer_help.h"
#include <arpa/inet.h>
#include <errno.h>
#include "finger_table.h"
#define TEST
#define COMMAND_LEN 15
#define GETSOCKETERRNO() (errno)
#define SOCKET int

#ifdef TEST
#include <string.h>
#endif

int main(int argc, char* argv[]){
    // Setup Peer Info
    peer_info self_info;
    payload * hash = NULL;
    payload * response_socket_head = NULL;
    self_info.hash_head = &hash;
    self_info.response_sockets_head = &response_socket_head;
    self_info.states = listCreate();

    if(setup_peer_info(&self_info, argv, argc) == -1) {
        exit(EXIT_FAILURE);
    }
    #ifdef TEST
        printf("started peer :\n");
        print_peer_info_long(&self_info);
    #endif
    // setup connection
    SOCKET listen_sock = setup_listen_socket(self_info.self_port, ip_string);
    if(listen_sock == -1) {
        fprintf(stderr, " - setup_listen_socket\n");
        exit(EXIT_FAILURE);
    }

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    bool running = true;

    fd_set connections_storage;
    FD_ZERO(&connections_storage);

    FD_SET(listen_sock, &connections_storage);
    SOCKET max_socket = listen_sock;

    #ifdef TEST
        printf("\ncommand : \n");
        fflush(stdout);
    #endif
    // Start Join Process
    if(!self_info.first_peer) {
        // Send Join
        internal_message *join_msg = new_internal_message(JOIN, 0, self_info.self_id, self_info.self_ip, self_info.self_port);
        int peer_sock = connect_to_peer(self_info.join_ip, self_info.join_port);
        if(send_internal_message(join_msg, peer_sock) == -1) {
            fprintf(stderr, " Sending Join to Entry Node\n");
            exit(EXIT_FAILURE);
        }
        close(peer_sock);
    }

    while(running) {
        // copy FD set
        fd_set in_fd = connections_storage;
        // value 0 = wait until a socket is ready to get read from
        int rv = select(max_socket+1, &in_fd, NULL, NULL, &tv);

        if (rv == -1) {
            // select modifies the input set
            fprintf(stderr, "Peer: select() failed. (%d)\n", GETSOCKETERRNO());
            perror("\n");
            exit(EXIT_FAILURE);
        }
        // Timout:
        if (rv == 0) {
            if (self_info.initialised_next){
                // Send Stabalize
                internal_message *stabalize = new_internal_message(STABILIZE, 0, self_info.self_id, self_info.self_ip, self_info.self_port);
                int peer_sock = connect_to_peer(self_info.next_ip, self_info.next_port);
                if(send_internal_message(stabalize, peer_sock) == -1) {
                    fprintf(stderr, " Sending Stabalize\n");
                    exit(EXIT_FAILURE);
                }
                close(peer_sock);

            }
            continue;
        }
        SOCKET i;
        for(i = 0; i <= max_socket; ++i) {
            // FD_ISSET is true if a socket was flagged as ready from select
            if (FD_ISSET(i, &in_fd)) {
                // if the current socket is a listening socket, we are going to accept()
                if (i == listen_sock) {
                    struct sockaddr_storage client_addr;
                    socklen_t addr_size = sizeof client_addr;
                    // Wait for new Connections
                    SOCKET client_sock = accept(listen_sock, (struct sockaddr *) &client_addr, &addr_size);
                    if (client_sock == -1) {
                        fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
                        continue;
                    }

                    FD_SET(client_sock, &connections_storage);

                    if (client_sock > max_socket) {
                        max_socket = client_sock;
                    }
                    #ifdef TEST
                        socklen_t len;
                        struct sockaddr_storage addr;
                        char ipstr[INET_ADDRSTRLEN];
                        int client_port;

                        len = sizeof addr;
                        getpeername(client_sock, (struct sockaddr*)&addr, &len);

                        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
                        client_port = ntohs(s->sin_port);
                        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

                        printf("\nNew connection from %d:%s at socket: %d\n", client_port, ipstr, client_sock);
                    #endif
                }
                else if(i == STDIN_FILENO){
                    // Shutdown
                    #ifdef TEST
                    char *command = calloc(COMMAND_LEN, sizeof(char));
                    fscanf(stdin, "%s", command);
                    fflush(stdin);
                    if(strcmp(command, "ft") == 0){
                        //TODO force to build ft
                        printf("build a finger table !\n");
                        create_ft(&self_info);
                        init_fill_ft(&self_info);
                    }
                    if(strcmp(command, "ft_print") == 0){
                        finger_table* FT = self_info.ft;
                        printf("Finger table !\n");
                        print_ft((finger_table*) self_info.ft);
                    }
                    if(strcmp(command, "stop") == 0){
                        running = false;

                    }
                    if(strcmp(command, "powi") == 0){
                        uint16_t x = 2;
                        uint16_t e = 10;
                        printf("%d to power of %d = %d", x, e, powi(x, e));

                    }
                    //i = max_socket + 1;
                    free(command);
                    continue;
                    #endif
                }
                else {
                    message* m_in = malloc(sizeof(message));
                    // Receive and Decode Message
                    if(recv_message(m_in, i) == -1) {
                        FD_CLR(i, &connections_storage);
                        close(i);
                        continue;
                    }
                    #ifdef TEST
                    printf("recieved: \n");
                    print_message(m_in);
                    fflush(stdout);
                    fflush(stdin);
                    #endif
                    react_on_incoming_message(m_in, &self_info, i, &connections_storage);
                    //message is being freed in reac_on bla bla
                    //free_message(m_in);
                }
            }
        }
    }
    if(self_info.ft != NULL){
        if( ((finger_table*)self_info.ft)->filled)free_ft(self_info.ft);
    }
    h_free(self_info.hash_head);
    h_free(self_info.response_sockets_head);
    exit(EXIT_SUCCESS);
}

