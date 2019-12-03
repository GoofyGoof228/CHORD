#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include "hash_table.h"
#include "peer_netw.h"
#include "peer_help.h"
#include <arpa/inet.h>
#include <errno.h>
//#define TEST
#define GETSOCKETERRNO() (errno)
#define SOCKET int

#define DG


int main(int argc, char* argv[]){
    if(argc != 10){
        fprintf(stderr,"Usage: ./peer (self) id ip port (previous) id ip port (next) id ip port\n");
        exit(EXIT_FAILURE);
    }
    // TO DO setup
    peer_info self_info;
    payload * hash = NULL;
    payload * response_socket_head = NULL;
    self_info.hash_head = &hash;
    self_info.response_sockets_head = &response_socket_head;
    self_info.states = listCreate();


    self_info.self_id = atoi(argv[1]);
    self_info.self_ip = get_ipv4_addr(argv[2]);
    self_info.self_port = atoi(argv[3]);

    self_info.previous_id = atoi(argv[4]);
    self_info.previous_ip = get_ipv4_addr(argv[5]);
    self_info.previous_port = atoi(argv[6]);

    self_info.next_id = atoi(argv[7]);
    self_info.next_ip = get_ipv4_addr(argv[5]);
    self_info.next_port = atoi(argv[9]);
#ifdef TEST
    printf("started peer :\n");
    print_peer_info_long(&self_info);
#endif



    // setup connection
    char port[12];

    sprintf(port, "%d", self_info.self_port);
    struct addrinfo hints, *result, *p;
    SOCKET listen_sock = 0;
    int yes = 1;
    memset(&hints, 0, sizeof(hints));
    /**
     * when set to AF_INET doest recieve connections from localhost
     * but ew why ????
     * */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &result) != 0) {
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
            exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
    if (listen(listen_sock, 10) == -1) {
        perror("Server: at listen");
        exit(EXIT_FAILURE);
    }
    fd_set connections_storage;
    FD_ZERO(&connections_storage);
    FD_SET(listen_sock, &connections_storage);
    SOCKET max_socket = listen_sock;
    while(1) {
        // copy filediscriptor set
        fd_set in_fd = connections_storage;
        // value 0 = wait until a socket is ready to get read from
        if (select(max_socket+1, &in_fd, NULL, NULL, NULL) < 0) {
            // select modifies the input set
            fprintf(stderr, "Peer: select() failed. (%d)\n", GETSOCKETERRNO());
            perror("\n");
            return -1;
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
                else {
                    message* m_in = malloc(sizeof(message));
                    // Receive and Decode Message
                    if(recv_message(m_in, i) == -1){
                        FD_CLR(i, &connections_storage);
                        close(i);
                        continue;
                    }
                    react_on_incoming_message(m_in, &self_info, i, &connections_storage);
                    free(m_in);
                }
            }
        }
    }
    h_free(self_info.hash_head);
    h_free(self_info.response_sockets_head);
    exit(EXIT_SUCCESS);
}

