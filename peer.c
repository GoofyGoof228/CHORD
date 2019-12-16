#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "hash_table.h"
#include "peer_netw.h"
#include "peer_help.h"
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include "finger_table.h"
#include <sys/select.h>
#define TEST
#define COMMAND_LEN 15
#define GETSOCKETERRNO() (errno)
#define SOCKET int
#define FT_KEEP_ALIVE
#ifdef TEST
#include <string.h>
#endif
#define FT_M
#define LOG_SN 0


int main(int argc, char* argv[]){
    // Setup Peer Info
    peer_info self_info;
    payload * hash = NULL;
    payload * response_socket_head = NULL;
    self_info.hash_head = &hash;
    self_info.response_sockets_head = &response_socket_head;
    self_info.internal_states = listCreate();
    self_info.external_states = listCreate();

    if(setup_peer_info(&self_info, argv, argc) == -1) {
            exit(EXIT_FAILURE);
    }
    #ifdef TEST
        //print_peer_info_long(&self_info);
        printf("I: %s\n", peer_info_to_str(&self_info));
    #endif

    // setup connection
    SOCKET listen_sock = setup_listen_socket(self_info.self_port, argv[1]);
    if(listen_sock == -1) {
        fprintf(stderr, " - setup_listen_socket\n");
        exit(EXIT_FAILURE);
    }

    struct timeval time_out;
    time_out.tv_sec = 2;
    time_out.tv_usec = 0;

    bool running = true;

    fd_set connections_storage;
    FD_ZERO(&connections_storage);

    FD_SET(listen_sock, &connections_storage);
    SOCKET max_socket = listen_sock;

    FD_SET(STDIN_FILENO, &connections_storage);
    if(STDIN_FILENO > max_socket){
        max_socket = STDIN_FILENO;
    }
    //Start Join Process
    if (!self_info.first_peer) {
        // Send Join
        internal_message *join_msg = new_internal_message(JOIN, 0, self_info.self_id, self_info.self_ip, self_info.self_port);
        #ifdef TEST
            char addrr[20];
            struct in_addr inAddr;
            inAddr.s_addr = self_info.join_ip;
            inet_ntop(AF_INET, &inAddr, addrr, INET_ADDRSTRLEN);
            printf("trying to connect to : ip - %s, port - %d\n", addrr, self_info.join_port);
        #endif
        int peer_sock = connect_to_peer(self_info.join_ip, self_info.join_port);
        if(peer_sock == -1){
            exit(EXIT_FAILURE);
        }
        if (send_internal_message(join_msg, peer_sock) == -1) {
            fprintf(stderr, " Sending Join to Entry Node\n");
            exit(EXIT_FAILURE);
        }
        close_socket(peer_sock);
    }

    time_t last_stab_time = time(NULL);
    //self_info.connection_storage = &connections_storage;
    while(running) {
        // copy FD set
        fd_set in_fd = connections_storage;

        if (select(max_socket+1, &in_fd, NULL, NULL, &time_out) == -1) {
            fprintf(stderr, "Peer: select() failed. (%d)\n", GETSOCKETERRNO());
            perror("\n");
            exit(EXIT_FAILURE);
        }
        // Timeout or select has returned 5 times without sending a Stabalize:
        if (time(NULL) - last_stab_time >= 2) {
            if (self_info.initialised_next){
                // Send Stabalize
                internal_message *stabalize = new_internal_message(STABILIZE, 0, self_info.self_id, self_info.self_ip, self_info.self_port);
                int peer_sock = connect_to_peer(self_info.next_ip, self_info.next_port);
                if(peer_sock == -1) {
                    fprintf(stderr, "Error Connecting to Next Peer (Port: %u) to send Stabalize\n", self_info.next_port);
                    exit(EXIT_FAILURE);
                }
                if(send_internal_message(stabalize, peer_sock) == -1) {
                    fprintf(stderr, "Error Sending Stabalize\n");
                    exit(EXIT_FAILURE);
                }
                close_socket(peer_sock);
            }
            last_stab_time = time(NULL);
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
                    #ifdef TEST1
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
                }else if(i != STDIN_FILENO){
                    message* m_in = malloc(sizeof(message));
                    // Receive and Decode Message
                    if(recv_message(m_in, i) == -1) {
                        FD_CLR(i, &connections_storage);
                        close_socket(i);
                        continue;
                    }
                    #ifdef TEST
                    if(m_in->int_msg != NULL){
                        //if(m_in->int_msg->type != STABILIZE){
                        if(LOG_SN){
                            printf("R: %s\n", internal_message_to_str(m_in->int_msg));
                        } else if (m_in->int_msg->type != NOTIFY && m_in->int_msg->type != STABILIZE && m_in->int_msg->type != JOIN){
                            printf("R: %s\n", internal_message_to_str(m_in->int_msg));
                        }

                            //print_message(m_in);
                        //}
                    }
                    fflush(stdout);
                    fflush(stdin);
                    #endif

                    react_on_incoming_message(m_in, &self_info, i, &connections_storage);
                    //message is being freed in reac_on bla bla
                    //free_message(m_in);
                }else{
                    // Shutdown
                    #ifdef TEST
                        char *command = calloc(COMMAND_LEN, sizeof(char));
                        fscanf(stdin, "%s", command);
                        fflush(stdin);
                        if(strcmp(command, "ft") == 0){
                            //TO DO force to build ft
                            #ifndef FT_KEEP_ALIVE
                            internal_message* ft = new_internal_message(FINGER, self_info.self_id, self_info.self_id, self_info.self_ip, self_info.self_port);
                            SOCKET peer_socket = connect_to_peer(self_info.self_ip, self_info.self_port);
                            send_internal_message(ft, peer_socket);
                            close_socket(peer_socket);
                            free(ft);
                            #endif
                            #ifdef FT_KEEP_ALIVE
                            internal_message* ft = new_internal_message(FINGER, 0, 0, 0, 0);
                            SOCKET peer_socket = connect_to_peer(self_info.self_ip, self_info.self_port);
                            send_internal_message(ft, peer_socket);
                            free(ft);
                            FD_SET(peer_socket, &connections_storage);
                            if (peer_socket > max_socket){
                                max_socket = peer_socket;
                            }
                            #endif
                        }
                        if(strcmp(command, "fl") == 0){
                            //print FT in file
                            print_ft_in_file((finger_table*) self_info.ft);
                        }
                        if(strcmp(command, "fp") == 0){
                            print_ft((finger_table*) self_info.ft);
                        }
                        if(strcmp(command, "s") == 0){
                            running = false;
                            //break;
                        }
                        if(strcmp(command, "i") == 0){
                            print_peer_info_long(&self_info);
                        }
                        //i = max_socket + 1;
                        free(command);
                        continue;
                    #endif
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

