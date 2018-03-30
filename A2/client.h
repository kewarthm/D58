#ifndef _CSCD58W18_CLIENT_H_
#define _CSCD58W18_CLIENT_H_
// This DEST is for socket connections
// Which we are pretending that is the physical layer
// Sending to 255.255.255.255 is for broadcast
#define CSCD58W18_CLIENT_PORT "35410"

// Architecture
// Our custom communication protocol - Internet Layer
// Socket Network                    - Physical Layer
// Explanation:
//      Socket connections are created as if they are physical connections
//      Then a customized communication protocol (IP-like) was used
//      as a internet layer

#include "common.h"


struct cscd58w18_client {
    int connection_fd;
    char ip[16];
    int (*start)(struct cscd58w18_client *this);
    int (*send)(struct cscd58w18_end_point *this, void *buffer, size_t len);
    int (*recv)(struct cscd58w18_end_point *this, void *buffer, size_t len);
    char padding[4];
};


void client_init(struct cscd58w18_client *this, const char *ip);
int client_start(struct cscd58w18_client *this);

#endif
