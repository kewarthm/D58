#ifndef _CSCD58W18_SERVER_H_
#define _CSCD58W18_SERVER_H_
#define CSCD58W18_SERVER_DEST "255.255.255.255"
#define CSCD58W18_SERVER_PORT "35410"
#define CSCD58W18_SERVER_QUEUE_LIMIT 5

#include "common.h"

struct cscd58w18_server {
    int listening_fd;
    char ip[16];
    int (*start)(struct cscd58w18_server *this);
    int (*send)(struct cscd58w18_end_point *this, void *buffer, size_t len);
    int (*recv)(struct cscd58w18_end_point *this, void *buffer, size_t len);
	int (*connection_handler)(struct cscd58w18_server *this, int connection_fd);
};


void server_init(struct cscd58w18_server *this, const char *ip);
int server_start(struct cscd58w18_server *this);



#endif
