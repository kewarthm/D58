#include "client.h"

void client_init(struct cscd58w18_client *this, const char *ip) {
    strcpy(this->ip, ip);
    this->start = client_start;
    this->send = send_all;
    this->recv = recv_all;
}
int client_start(struct cscd58w18_client *this) {
	// Initialize client and prepares for connection
	struct addrinfo hints;
	struct addrinfo *res;
	int listening_fd;
	int rslt = 0;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	
	// Fill in addrinfo automatically
	int rv = 0;
	if(rv = getaddrinfo(this->ip, CSCD58W18_CLIENT_PORT, &hints, &res)) {
        fprintf(stderr, "client: getaddrinfo - %s\n", gai_strerror(rv));
        return 1;
	}

    // loop through all the results and connect to the first we can
    struct addrinfo *p = NULL;
    for(p = res; p != NULL; p = p->ai_next) {
        if ((this->connection_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(this->connection_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(this->connection_fd);
            perror("client: connect");
            continue;
        }

        break;
    }
	freeaddrinfo(res); // Done with addrinfo
	
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    // Getting address
    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);
    
    // Handshake
    // Test connection by exchanging ip
    printf("client: send handshake.....");
    if(this->send((struct cscd58w18_end_point *)this, s, INET6_ADDRSTRLEN) <= 0) {
        printf("error!\n");
        fprintf(stderr, "client: failed to send handshake\n");
        rslt = 3;
    } else {
        printf("ok!\n");
    }
    printf("client: recv handshake.....");
    if(this->recv((struct cscd58w18_end_point *)this, s, INET6_ADDRSTRLEN) <= 0) {
        printf("error!\n");
        fprintf(stderr, "client: failed to recv handshake\n");
        rslt = 3;
    } else {
        printf("ok!\n");
    }
    
    if(!rslt) {
        printf("client: successfully connected to %s\n", s);
    }

	return rslt;
}
