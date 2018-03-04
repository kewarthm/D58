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

#define HEADER_SIZE_LIMIT 2048
#define BUFFER_SIZE 2048

void clean_exit(int rc, int fd, char *message);
int read_file(const char *root, const char* path, char** out_dataPtr);
int supportedMethod(const char *methodName);
int supportedVersion(const char *httpVersion);
int listen_requests(int sockfd);
int recvLine(int sockfd, char *buffer);
int sendAll(int sockfd, const char *buffer, int size);
int handle_field(int sockfd, char *key, char *value);
const char *get_file_extension(char *filename);
const char *get_mime_type(char *filename);
int handle_requests(int sockfd);
int init_server(const char *PORT, const char *ADDR, const int QUEUE_LIMIT, struct addrinfo *out_serverAddrInfo);

char *rootPath = NULL;
int main(int argc, char * argv[]){
	/*
	int error = 0;
	int exit = 0;
	init_server();
	while(!exit) {
		exit = handle_requests();
	}
	
	
	*/
	
	
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
	opt = 1;
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
					write(client_fd, "HTTP/1.0 400 Bad Request\n", 25);
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
					else write(client_fd, "HTTP/1.0 404 Not Found\n", 23);	
				}
			}
		}
		
		// close client
		close(client_fd);
	}

}

void clean_exit(int rc, int fd, char *message) {
	if (rc == -1 || fd == -1){
		if(fd != -1){		
			close(fd);
		}
		perror(message);
		exit(EXIT_FAILURE);
	}
}

int read_file(const char *root, const char* path, char** out_dataPtr) {
	// Detect ".." in path to prevent access to critical files
	// if ".." detected
	//		return 1
	// Read data
	// If read data file
	//		return 1
	// Allocate memory and make "out_dataPtr" points to it
	// return 0
}

int supportedMethod(const char *methodName) {
	return (strcmp(methodName, "GET") == 0);
}

int supportedVersion(const char *httpVersion) {
	return (strcmp(httpVersion, "HTTP/1.0") == 0);
}

int listen_requests(int sockfd) {
	struct sockaddr_storage connection_addr;
	socklen_t addr_size = sizeof connection_addr;
	// wait for connection, accept()
	int connection_fd = accept(sockfd, (struct sockaddr *)&connection_addr, &addr_size);
	
	// Spawn worker process to handle requests
	int status = fork();
	if(status == 0) {
		// Connection already established when accept() returned..
		// Handle incoming request..
		handle_requests(connection_fd);
		//exit(0);
	} else if(status < 0) {
		// Failed to create worker process 
		perror("Failed to create worker process; memory exhaustion?");
		send(connection_fd, "HTTP/1.0 503 Service Unavailable\n", 33, 0);
		close(connection_fd);
		return 1;
	}
	return 0;
}

int recvLine(int sockfd, char *buffer) {
	int count = 0;
	int rcvd = 0;
	do {
		rcvd = recv(sockfd, buffer++, 1, 0);
		++count;
		// Return on CRLF or 0 bytes being read..
	} while(rcvd > 0 && (*buffer != '\n' || *(buffer-1) != '\r'));
	return count;
}

// Normal send does not ensure it sends all data at once
// This function ensures that..
// Size <= 0 means determine the size automatically for null terminated strings
// Otherwise only send the specified amount of bytes
// Returns -1 if error
int sendAll(int sockfd, const char *buffer, int size) {
	int len = size > 0 ? size : strlen(buffer);
	int i = 0;
	
	// If not all bytes are sent, try to send the rest..
	do {
		i = send(sockfd, buffer, len - i, 0);
		buffer += i;
		if(i <= 0) {
			return i;
		}
	} while(len - i > 0);
	return 0;
}

int handle_field(int sockfd, char *key, char *value) {
	if(strcmp(key, "???") == 0) {
		
	} else {
		return -1;
	}
	return 0;
}

const char *get_file_extension(char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

const char *get_mime_type(char *filename) {
	const char* ext = get_file_extension(filename);
	if(strcmp(ext, "js") == 0){
		return "text/javascript";
	} else if(strcmp(ext, "css") == 0) {
		return "text/css";
	} else if(strcmp(ext, "jpg") == 0) {
		return "image/jpeg";
	} else if(strcmp(ext, "html") == 0) {
		return "text/html";
	} else {
		return "text/plain";
	}
}


int handle_requests(int sockfd) {
#define HEADER_SIZE_LIMIT 2048
#define BUFFER_SIZE 2048
	int rcvdTotal = 0; // Total received..
	int rcvd = 0;
	int requireBody = 1;
	char buffer[BUFFER_SIZE] = {0};
	char file_path[HEADER_SIZE_LIMIT] = {0}; // TODO: Bug, root path + resource path may result index out of bound
	char *key = NULL;
	char *value = NULL;
	char *version = NULL;
	
	// Read first line in data until hitting CRLF
	rcvd = recvLine(sockfd, buffer);
	if(rcvd == 0) {
		perror("Connection closed unexpectedly or timed out");
		close(sockfd);
		return -1;
	}
	
	// Split into key, value pair.
	// Value should contain method name and HTTP version
	key = strtok(buffer, " \t\n");
	// Test supported method
	// Note: Whether method is allowed on the given resource is another story
	if(!supportedMethod(key)) {
		sendAll(sockfd, "HTTP/1.0 405 Method Not Allowed\n", 0);
		close(sockfd);
		return -1;
	}
	
	value = strtok(NULL, " \t\n");
	
	// Test HTTP version
	version = strtok(NULL, " \t\n");
	if(!supportedVersion(version)) {
		// Return error code 505 if version not supported..
		sendAll(sockfd, "HTTP/1.0 505 Method Not Allowed\n", 0);
		close(sockfd);
		return -1;
	}
	
	
	// Try to find the file
	strcpy(file_path, value);
	strcpy(&file_path[strlen(rootPath)], value);
	int fd;
	if((fd=open(file_path, O_RDONLY)) != -1){
		sendAll(sockfd, "HTTP/1.0 200 OK\n", 0);
		sendAll(sockfd, "content-type: ", 0);
		sendAll(sockfd, get_mime_type(file_path), 0);
	} else {
		sendAll(sockfd, "HTTP/1.0 404 Not Found\n", 0);
	}
	
	// for line in data
	while(recvLine(sockfd, buffer) > 0) {
		printf("%s\n", buffer);
		key = strtok(buffer, ":\r\n");
		value = strtok(buffer, ":\r\n");
		
		if(handle_field(sockfd, key, value)) {
			// Unsupported header
			fprintf(stderr, "Unsupported header: %s", buffer);
		}
	}
	
	// send response body
	if(requireBody) {
		char fileBuffer[BUFFER_SIZE] = {0};
		int bytes_read;
		while((bytes_read=read(fd, fileBuffer, 1024)) > 0){
			sendAll(sockfd, fileBuffer, bytes_read);
		}
	}
	
	close(sockfd);
}

// Initializes server
//  	port number - string
//  	address - string
//  	out_serverAddrInfo - buffer of type struct addrinfo to hold output information
// Returns:
//		File Descriptor for listening socket
int init_server(const char *PORT, const char *ADDR, const int QUEUE_LIMIT, struct addrinfo *out_serverAddrInfo) {
	// Initialize server and prepares for listening 
	struct addrinfo hints;
	struct addrinfo *res;
	int listening_fd;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	
	// Fill in addrinfo automatically
	getaddrinfo(ADDR, PORT, &hints, &res);
	memcpy(out_serverAddrInfo, res, sizeof(struct addrinfo));
	freeaddrinfo(res); // Addr Info is now saved, we can release it
	res = out_serverAddrInfo;
	
	// Create socket, which prepares a connection
	listening_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	// Bind to a port and address, with the given information
	bind(listening_fd, res->ai_addr, res->ai_addrlen);
	
	// Start listening on the specified address and port
	listen(listening_fd, QUEUE_LIMIT);
	
	return listening_fd;
}