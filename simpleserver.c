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
	
	
	// Minimal necessary variables to initialize server
	int listening_fd;
	struct addrinfo serverAddrInfo;
	
	// requires a port number to listen on and a directory path
	if(argc != 3){
		fprintf(stderr, "[Usage]: %s PORT ROOT_PATH\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	rootPath = argv[2];
	
	listening_fd = init_server(argv[1], NULL, 5, &serverAddrInfo);
	
	while(1) {
		listen_requests(listening_fd);
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
	return (strncmp(methodName, "GET", 3) == 0);
}

int supportedVersion(const char *httpVersion) {
	return (strncmp(httpVersion, "HTTP/1.0", 8) == 0);
}

int listen_requests(int sockfd) {
	struct sockaddr_storage connection_addr;
	socklen_t addr_size = sizeof connection_addr;
	// wait for connection, accept()
	int connection_fd = 0;
	
	// Try again until get an good connection
	while((connection_fd = accept(sockfd, (struct sockaddr *)&connection_addr, &addr_size)) <= 0) {
		// TODO: Add error handling by checking "errno"
		// There maybe some fatal errors
		// switch(errno) {
		//
		// }
	}
	
	printf("Incoming connection\n");
	
	// Spawn worker process to handle requests
	int status = fork();
	if(status == 0) {
		// Child process also receives a listening socket
		// Close it..
		
		// Connection already established when accept() returned..
		// Handle incoming request..
		handle_requests(connection_fd);
		close(sockfd);
		exit(0);
	} else if(status < 0) {
		// Failed to create worker process 
		perror("Failed to create worker process; memory exhaustion?");
		send(connection_fd, "HTTP/1.0 503 Service Unavailable\n", 33, 0);
		close(connection_fd);
		return 1;
	}
	// Parent process no longer talks to incoming connection..
	close(connection_fd);
	return 0;
}

int recvLine(int sockfd, char *buffer) {
	int count = 0;
	int rcvd = 0;
	do {
		rcvd = recv(sockfd, buffer++, 1, 0);
		++count;
		// Return on CRLF or 0 bytes being read..
	} while(rcvd > 0 && (*(buffer-1) != '\n' || *(buffer-2) != '\r'));
	return rcvd > 0 ? count : rcvd;
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
	if(strncmp(key, "???", 3) == 0) {
		
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
	if(strncmp(ext, "js", 2) == 0){
		return "text/javascript";
	} else if(strncmp(ext, "css", 3) == 0) {
		return "text/css";
	} else if(strncmp(ext, "jpg", 3) == 0) {
		return "image/jpeg";
	} else if(strncmp(ext, "html", 4) == 0) {
		return "text/html";
	} else {
		return "text/plain";
	}
}


int handle_requests(int sockfd) {
	int rcvdTotal = 0; // Total received..
	int rcvd = 0;
	int requireBody = 1;
	char buffer[BUFFER_SIZE] = {0};
	char file_path[HEADER_SIZE_LIMIT] = {0}; // TODO: Bug, root path + resource path may result index out of bound
	char *key = NULL;
	char *value = NULL;
	char *version = NULL;

	// Print IP Address
	socklen_t len;
	struct sockaddr_storage addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;

	len = sizeof addr;
	getpeername(sockfd, (struct sockaddr*)&addr, &len);

	// deal with both IPv4 and IPv6:
	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	printf("Peer IP address: %s\n", ipstr);
	// End print IP Address
	
	// Read first line in data until hitting CRLF
	rcvd = recvLine(sockfd, buffer);
	if(rcvd <= 0) {
		perror("Connection closed unexpectedly or timed out");
		close(sockfd);
		return -1;
	}
	printf(" * Received: %s", buffer);
	
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
		sendAll(sockfd, "HTTP/1.0 505 Version Not Supported\n", 0);
		close(sockfd);
		return -1;
	}
	
	
	// Try to find the file
	strcpy(file_path, rootPath);
	strcpy(&file_path[strlen(rootPath)],
			strncmp(value, "/\0" ,2) == 0 ? "/index.html" : value);
	int fd;
	if((fd=open(file_path, O_RDONLY)) != -1){
		sendAll(sockfd, "HTTP/1.0 200 OK\r\n", 0);
		sendAll(sockfd, "content-type: ", 0);
		sendAll(sockfd, get_mime_type(file_path), 0);
		sendAll(sockfd, "\r\n", 0);
	
		// for line in data
		// If data read is empty line with CRLF only..
		// Then it is end of the header
		while((rcvd = recvLine(sockfd, buffer)) > 0 && strncmp(buffer, "\r\n", 2) != 0) {
			buffer[rcvd] = '\0';
			printf("%s", buffer); // Each line of valid header is guarantteed to end with CRLF
			key = strtok(buffer, ":\r\n");
			value = strtok(NULL, ":\r\n");
			
			if(handle_field(sockfd, key, value)) {
				// Unsupported header
				fprintf(stderr, "Unsupported header: (%s: %s)\n", key, value);
			}
		}
		
		// End of response header
		sendAll(sockfd, "\r\n", 0);
		
		// send response body
		char fileBuffer[BUFFER_SIZE] = {0};
		int bytes_read;
		while((bytes_read=read(fd, fileBuffer, 1024)) > 0){
			sendAll(sockfd, fileBuffer, bytes_read);
		}
		// End of response body
		sendAll(sockfd, "\r\n", 0);
	} else {
		sendAll(sockfd, "HTTP/1.0 404 Not Found\n\n", 0);
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
	int rslt = 0;
	
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
	
	// Allow socket reuse, thus not blocking port if this program fails
	int opt = 1;
	rslt = setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, &opt, (socklen_t)sizeof(int));
	
	// Set timeout, prevent waiting forever and "slow down" resource exhaustion
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	rslt = setsockopt(listening_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	
	// Bind to a port and address, with the given information
	rslt = bind(listening_fd, res->ai_addr, res->ai_addrlen);
	
	// Start listening on the specified address and port
	rslt = listen(listening_fd, QUEUE_LIMIT);
	
	return listening_fd;
}