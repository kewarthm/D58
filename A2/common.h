#ifndef _CSCD58W18_A2_COMMON_H_
#define _CSCD58W18_A2_COMMON_H_

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>

struct cscd58w18_end_point {
    int sockfd;
    char ip[16];
    char padding[16]; // 4 bytes per function pointer + 4 bytes reserved
};

void *get_in_addr(struct sockaddr *sa);
int send_all(struct cscd58w18_end_point *this, void *buffer, size_t len);
int recv_all(struct cscd58w18_end_point *this, void *buffer, size_t len);
unsigned int ip_to_int(char *ip);
int int_to_ip(unsigned int ip_in_int, char buffer[INET6_ADDRSTRLEN]);


#endif
