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

void clean_exit(int rc, int fd, char *message){
	if (rc == -1 || fd == -1){
		if(fd != -1){		
			close(fd);
		}
		perror(message);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char * argv[]){
	int server_fd, client_fd, rc, client_addr_len, opt, rcvd, fd, bytes_read;
	struct sockaddr_in server_addr, client_addr;
	char client_msg[1024], *server_msg = "Hello Client\n", *http_root_path, *reqline[5], data_to_send[1024];

	if(argc != 3){
		fprintf(stderr, "[Usage]: %s PORT ROOT_PATH\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	http_root_path = argv[2];

	// create server address struct
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	server_addr.sin_addr.s_addr = INADDR_ANY; // bind to an available IP

	// open socket and check error
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	clean_exit(server_fd, server_fd, "[Server socket error]: ");

	// allow reusable port after disconnet or termination of server
	rc = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, (socklen_t)sizeof(int));
	clean_exit(rc, server_fd, "[Server setsocket error]: ");

	// bind socket to any address on machine and port and check error
	rc = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
	clean_exit(rc, server_fd, "[Server bind error]: ");

	// listen on socket for up to 5 connections
	rc = listen(server_fd, 5);
	clean_exit(rc, server_fd, "[Server listen error]: ");

	// Accept connection forever
	printf("SERVER AT %s:%d LISTENING FOR CONNECTIONS\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	while(1){
		/*
		Persistent Connection Code goes here:
		 - Need forking code from Dennis
		 - Need to maintain connection until timeout (5 secs?) or until keep-alive time
		 - The Keep-Alive header MUST be ignored if received without the connection token.
		 - Connection: keep-alive | Keep connection alive for Keep-Alive: value time or for default time if not provided
		 - Connection: close | This header must/will be sent to indictate the connection will closed after this transaction

		*/
	}
}
