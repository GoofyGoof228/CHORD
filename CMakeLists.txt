cmake_minimum_required(VERSION 3.13)
project(Block5)

set(CMAKE_CXX_STANDARD 14)

include_directories(include source)

#[[set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -g")
set(CMAKE_C_LINKER_FLAGS "${CMAKE_C_LINKER_FLAGS}$ -fsanitize=undefined -g")]]#
add_executable(peer peer.c
        include/external_message.h
        include/hash_table.h
        include/internal_message.h
        include/list.h
        include/message.h
        include/payload.h
        include/peer_help.h
        include/peer_netw.h
        include/uthash.h
        include/finger_table.h
        source/external_message.c
        source/hash_table.c
        source/internal_message.c
        source/list.c
        source/message.c
        source/payload.c
        source/peer_help.c
        source/peer_netw.c
        source/finger_table.c
        )


