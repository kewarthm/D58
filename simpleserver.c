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
	// some variables
	int server_fd, client_fd, rc, client_addr_len, opt, rcvd, fd, bytes_read;
	struct sockaddr_in server_addr, client_addr;
	char client_msg[1024], *server_msg = "Hello Client\n", *http_root_path, *reqline[3], file_path[1024], data_to_send[1024];
	
	// requires a port number to listen on and a directory path
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
	printf("SERVER AT %s:%d LISTENING FOR CONNECTIONS", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	while(1){
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
		clean_exit(client_fd, server_fd, "[Server accept error]: ");
		
		// process requests
		rcvd = recv(client_fd, client_msg, 1024, 0);
		if(rcvd < 0) clean_exit(rc, server_fd, "[Server recv error]: ");
		else if(rcvd == 0) clean_exit(rc, server_fd, "[Client disconnected unexpectedly]: ");
		else{
			// successful msg
			reqline[0] = strtok (client_msg, " \t\n");
			// Look if msg was a HTTP GET request
			if (strncmp(reqline[0], "GET\0", 4) == 0){
				reqline[1] = strtok(NULL, " \t");
				reqline[2] = strtok(NULL, " \t\n");
				// check if msg was using the HTTP 1.0 protocol
				// we can expand this to include 1.1 later
				if(strncmp(reqline[2], "HTTP/1.0", 8) != 0){
					write(client_fd, "HTTP1.0 400 Bad Request\n", 25);
				}
				else{
					// no file was specified, provide default file
					if(strncmp(reqline[1], "/\0" ,2) == 0){
						reqline[1] = "/index.html";
					}
					strcpy(file_path, http_root_path);
					strcpy(&file_path[strlen(http_root_path)], reqline[1]);
					printf("file: %s\n", file_path);
					
					// File found, send to client
					if((fd=open(file_path, O_RDONLY)) != -1){
						send(client_fd, "HTTP/1.0 200 OK\n\n", 17, 0);
						while((bytes_read=read(fd, data_to_send, 1024)) > 0){
							write(client_fd, data_to_send, bytes_read);
						}
					}
					else write(client_fd, "HTTP/1.0 404 Not Found", 23);	
				}
			}
			else write(client_fd, "HTTP1.0 400 Bad Request\n", 25);
		}
		
		// close client
		close(client_fd);
	}

}


