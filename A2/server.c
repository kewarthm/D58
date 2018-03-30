#include "server.h"

// For using pthread_tryjoin_np
#define _GNU_SOURCE
#include <pthread.h>

struct _connection_handler_wrapper_param {
	int connection_fd;
	struct cscd58w18_server *server;
};

void *_server_listen_thread(void *arg);
void *_connection_handler_wrapper(void *arg);

void server_init(struct cscd58w18_server *this, const char *ip) {
    strcpy(this->ip, ip);
    this->start = server_start;
    this->send = send_all;
    this->recv = recv_all;
}

int server_start(struct cscd58w18_server *this) {
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
	if(rv = getaddrinfo(NULL, CSCD58W18_SERVER_PORT, &hints, &res)) {
        fprintf(stderr, "server: getaddrinfo - %s\n", gai_strerror(rv));
        return 1;
	}

    // loop through all the results and connect to the first we can
    struct addrinfo *p = NULL;
    for(p = res; p != NULL; p = p->ai_next) {
        if ((this->listening_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

    	int yes = 1;
        if (setsockopt(this->listening_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                (socklen_t)sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(this->listening_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(this->listening_fd);
            perror("server: bind");
            continue;
        }

        break;
    }
	freeaddrinfo(res); // Done with addrinfo
	
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    
    if (listen(this->listening_fd, CSCD58W18_SERVER_QUEUE_LIMIT) == -1) {
        perror("listen");
        exit(1);
    }
	
    pthread_t thread_listen;
    pthread_create(&thread_listen, NULL, _server_listen_thread, this);

	return rslt;
}

void *_server_listen_thread(void *arg) {
	struct sockaddr_storage connection_addr;
	socklen_t addr_size = sizeof connection_addr;
	int connection_fd = 0;
	struct cscd58w18_server *server = arg;
    
	while(1) {
		while((connection_fd = accept(server->listening_fd, (struct sockaddr *)&connection_addr, &addr_size)) <= 0) {
			// TODO: Add error handling by checking "errno"
			// There maybe some fatal errors
			// switch(errno) {
			//
			// }
		}
		
		int rslt = 0;
		struct cscd58w18_end_point node = {0};
		node.sockfd = connection_fd;
		
		// Getting address
		char s[INET6_ADDRSTRLEN] = "000.000.000.000";
		//inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("server: connection detected..\n");
		
		// Handshake
		// Test connection by exchanging ip
		printf("server: send handshake.....");
		if(server->send(&node, s, INET6_ADDRSTRLEN) <= 0) {
			printf("error!\n");
			fprintf(stderr, "server: failed to send handshake\n");
			rslt = 3;
		} else {
			printf("ok!\n");
		}
		
		printf("server: recv handshake.....");
		if(server->recv(&node, s, INET6_ADDRSTRLEN) <= 0) {
			printf("error!\n");
			fprintf(stderr, "server: failed to recv handshake\n");
			rslt = 3;
		} else {
			printf("ok!\n");
		}
		
		if(!rslt) {
			printf("server: successfully connected to %s\n", s);
		} else {
			close(connection_fd);
			continue;
		}
		
		// Call user-defined connection handler
		pthread_t thread_connection;
		struct _connection_handler_wrapper_param param = { connection_fd, server };
		pthread_create(&thread_connection, NULL, _connection_handler_wrapper, &param);
		
		//server->connection_handler(server, connection_fd);
	}
}

void *_connection_handler_wrapper(void *arg) {
	struct _connection_handler_wrapper_param *param = arg;
	if(param->server->connection_handler) {
		param->server->connection_handler(param->server, param->connection_fd);
	} else {
		printf("server: connection callback handler is unspecified, connection closed\n");
		close(param->connection_fd);
	}
}