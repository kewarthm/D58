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
// If this is not defined, compiler will complain about strptime()
#define __USE_XOPEN
#include<time.h>


#define HEADER_SIZE_LIMIT 2048
#define BUFFER_SIZE 2048
//#define SPAWN_WORKER


// START HTTPResponseHeader
#define HEADER_VALUE_LENGTH 2048
#define HEADER_COMPILE_MAX_LENGTH 2048
#define HEADER_KEY_RESERVE_BYTES 2
#define HEADER_VAL_RESERVE_BYTES 3
#define HEADER_BODY_TYPE_NONE   0
#define HEADER_BODY_TYPE_STR    1
#define HEADER_BODY_TYPE_FILE   2

void statusCodeToStr(int statusCode, char *outBuf) {
	switch(statusCode) {
		case 200:
			strcpy(outBuf, "200 OK");
			break;
		case 304:
			strcpy(outBuf, "304 Not Modified");
			break;
		case 400:
			strcpy(outBuf, "400 Bad Request");
			break;
		case 404:
			strcpy(outBuf, "404 Not Found");
			break;
		case 405:
			strcpy(outBuf, "405 Method Not Allowed");
			break;
		case 500:
			strcpy(outBuf, "500 Internal Server Error");
			break;
		case 505:
			strcpy(outBuf, "505 Version Not Supported");
			break;
		default:
			strcpy(outBuf, "500 Internal Server Error");
			break;
	}
}

struct field {
	// Each key only holds 2047 bytes + \0
	char key[HEADER_VALUE_LENGTH];
	char value[HEADER_VALUE_LENGTH];
	struct field *next;
};

struct HTTPResponseHeader {
	char version[16]; // Version string upto 15 characters + \0
	short statusCode;
	struct field *fields;
	short bodyType;
	void *body;
	void (*setVersion)(struct HTTPResponseHeader *, const char *);
	struct field *(*setField)(struct HTTPResponseHeader *, const char *, const char *);
	struct field *(*getField)(struct HTTPResponseHeader *, const char *);
	void (*setBody)(struct HTTPResponseHeader *, void *, short);
    size_t (*compile)(struct HTTPResponseHeader *, char [HEADER_COMPILE_MAX_LENGTH]);
    void (*release)(struct HTTPResponseHeader *, int);
};

void headerInit(struct HTTPResponseHeader *header);
void _headerSetVersion(struct HTTPResponseHeader *header, const char *version);
struct field *_headerSetField(struct HTTPResponseHeader *header, const char *key, const char *value);
struct field *_headerGetField(struct HTTPResponseHeader *header, const char *key);
void _headerSetBody(struct HTTPResponseHeader *header, void *body, short bodyType);
size_t _headerCompile(struct HTTPResponseHeader *header, char buffer[HEADER_COMPILE_MAX_LENGTH]);
void _headerRelease(struct HTTPResponseHeader *header, int isDynamic);


void headerInit(struct HTTPResponseHeader *header) {
	header->setVersion = _headerSetVersion;
    header->setField = _headerSetField;
    header->getField = _headerGetField;
    header->setBody = _headerSetBody;
    header->compile = _headerCompile;
    header->release = _headerRelease;
    header->statusCode = 500;
    header->fields = NULL;
    header->bodyType = HEADER_BODY_TYPE_NONE;
    header->body = NULL;
}

void _headerSetVersion(struct HTTPResponseHeader *header, const char *version) {
	strncpy(header->version, version, 15);
}

struct field * _headerSetField(struct HTTPResponseHeader *header, const char *key, const char *value) {
    struct field *curr = header->fields;
    struct field **parentNext = &header->fields;
    size_t len_k = strlen(key);
    
    while(curr) {
        if(strncasecmp(curr->key, key, len_k) == 0) {
            strncpy(curr->value, value, HEADER_VALUE_LENGTH - HEADER_KEY_RESERVE_BYTES);
            return curr;
        }
        parentNext = &curr->next;
        curr = curr->next;
    }
    curr = malloc(sizeof(struct field));
    if(curr) {
        strncpy(curr->key,   key,   HEADER_VALUE_LENGTH - HEADER_KEY_RESERVE_BYTES);
        strncpy(curr->value, value, HEADER_VALUE_LENGTH - HEADER_VAL_RESERVE_BYTES);
        curr->next = NULL;
        *parentNext = curr;
        return curr;
    }
    return NULL;
}

struct field *_headerGetField(struct HTTPResponseHeader *header, const char *key) {
    struct field *curr = header->fields;
    while(curr) {
        if(strcasecmp(curr->key, key) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void _headerSetBody(struct HTTPResponseHeader *header, void *data, short bodyType) {
	header->bodyType = bodyType;
	header->body = data;
}

size_t _headerCompile(struct HTTPResponseHeader *header, char buffer[HEADER_COMPILE_MAX_LENGTH]) {
    static struct HTTPResponseHeader *_prevHeader = NULL;
    static struct field *_prevField = NULL;
    static int _statusCompiled = 0;
    static int _bodyCompiled = 0;
	static int _headerTerminated = 0;
	size_t len = 0;
    if(header) {
        _prevHeader = header;
        _prevField = header->fields;
        _statusCompiled = 0;
        _bodyCompiled = 0;
		_headerTerminated = 0;
    }
    
    if(_prevHeader) {
		buffer[0] = '\0';
        if(!_statusCompiled) {
			strcat(buffer, _prevHeader->version);
			strcat(buffer, " ");
            statusCodeToStr(_prevHeader->statusCode, buffer + strlen(_prevHeader->version) + 1);
            strcat(buffer, "\r\n");
			len = strlen(buffer);
			_statusCompiled = !0;
        } else if(_prevField) {
            struct field *curr = _prevField;
            strcat(buffer, curr->key);
            strcat(buffer, ": ");
            strcat(buffer, curr->value);
            strcat(buffer, "\r\n");
			len = strlen(buffer);
            _prevField = curr->next;
		} else if(!_headerTerminated) {
			len = 2;
			strcat(buffer, "\r\n");
			_headerTerminated = !0;
        } else if(_prevHeader->bodyType && !_bodyCompiled) {
            switch(_prevHeader->bodyType) {
                case HEADER_BODY_TYPE_STR:
                    strcat(buffer, (char *)_prevHeader->body);
					len = strlen(buffer);
					_bodyCompiled = !0;
                    break;
                case HEADER_BODY_TYPE_FILE:
					if(_prevHeader->body) {
						len = read(*((int *)_prevHeader->body), buffer, HEADER_COMPILE_MAX_LENGTH);
						if(len == 0) {
							_bodyCompiled = !0;
						}
					} else {
						fprintf(stderr, "HTTP response compile, file descriptor is null when body should be a file\n");
					}
                    break;
                default:
					_bodyCompiled = !0;
                    break;
            }
        }
    }
    return len;
}

void _headerRelease(struct HTTPResponseHeader *header, int isDynamic) {
    struct field *curr = NULL;
    while(curr = header->fields) {
        header->fields = curr->next;
        free(curr);
    }
    
    if(isDynamic) free(header);
}
// END HTTPResponseHeader

void clean_exit(int rc, int fd, char *message);
int read_file(const char *root, const char* path, char** out_dataPtr);
int supportedMethod(const char *methodName);
int supportedVersion(const char *httpVersion);
int listen_requests(int sockfd);
int recvLine(int sockfd, char *buffer);
int sendAll(int sockfd, const char *buffer, int size);
int handle_field(char *resPath, struct HTTPResponseHeader *resp, char *key, char *value);
const char *get_file_extension(char *filename);
const char *get_mime_type(char *filename);
int handle_requests(int sockfd);
int init_server(const char *PORT, const char *ADDR, const int QUEUE_LIMIT, struct addrinfo *out_serverAddrInfo);

char *rootPath = NULL;
int main(int argc, char * argv[]){
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
	
	int status = 0;
#ifdef SPAWN_WORKER
	// Spawn worker process to handle requests
	status = fork();
#endif
	if(status == 0) {
		// Connection already established when accept() returned..
		// Handle incoming request..
		handle_requests(connection_fd);
		
#ifdef SPAWN_WORKER
		// Child process also receives a listening socket
		// Close it..
		close(sockfd);
		
		// Terminates once finished..
		// We don't want 2 servers running at the same time..
		exit(0);
#endif
	} else if(status < 0) {
		// Failed to create worker process 
		perror("Failed to create worker process; memory exhaustion?");
		send(connection_fd, "HTTP/1.0 503 Service Unavailable\n", 33, 0);
		close(connection_fd);
		return 1;
	}
		
#ifdef SPAWN_WORKER
	// Parent process no longer talks to incoming connection..
	close(connection_fd);
#endif
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

int handle_field(char *resPath, struct HTTPResponseHeader *resp, char *key, char *value) {
	if(strncmp(key, "If-Modified-Since", 17) == 0) {	
		struct stat attr;
		stat(resPath, &attr);
		time_t modTime = attr.st_mtime;
		char buf[32];
		struct tm tm_ = *gmtime(&modTime);
		if(!strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm_)) {
			fprintf(stderr, "Time conversion failed: %s\n", buf);
			resp->statusCode = 500;
			resp->setBody(resp, NULL, HEADER_BODY_TYPE_NONE);
			return !0;
		} 
		// mktime had to be called after strftime..
		// Since it modifies "tm_" and changes time zone
		time_t modTimeGM = mktime(&tm_);
		if(!strptime(value, "%a, %d %b %Y %H:%M:%S %Z", &tm_)) {
			fprintf(stderr, "Time conversion failed: %s\n", value);
			resp->statusCode = 400;
			resp->setBody(resp, NULL, HEADER_BODY_TYPE_NONE);
			return !0;
		} 
		
		// Compute submitted time - modification time
		time_t delta = difftime(mktime(&tm_), modTimeGM);
		if(delta < 0) {
			// Server resource is more recent..
			// Send it..
			resp->setField(resp, "Last-Modified", buf);
		} else if(delta == 0) {
			// Server resource is not more recent..
			// No need to send again..
			resp->statusCode = 304;
			resp->setBody(resp, NULL, HEADER_BODY_TYPE_NONE);
			return !0;
		} else {
			// Invalid timestamp, requested resource is more recent than server
			resp->statusCode = 404;
			resp->setBody(resp, NULL, HEADER_BODY_TYPE_NONE);
			return !0;
		}
	} else if(strncmp(key, "If-Match", 8) == 0) {
		//if(strncpy(value, Etag, strlen(Etag) != 0)) {
		//	sendAll(sockfd, "HTTP/1.0 416 Range Not Satisfiable\n\n", 0);
		//	requireBody = 0;
		//}
	} else if(strncmp(key, "If-None-Match", 13) == 0) {
		
	} else if(strncmp(key, "If-Unmodified-Since", 19) == 0) {
		
	} else if(strncmp(key, "If-Range", 8) == 0) {
		
	} else {
		// Unsupported header
		fprintf(stderr, "Unsupported header: (%s: %s)\n", key, value);
		return !0;
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
	char version[8] = {0};
	struct HTTPResponseHeader resp = {0};
	headerInit(&resp);

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
	key = strtok(buffer, " \t\r\n");
	value = strtok(NULL, " \t\r\n");	// Resource path
	strncpy(version, strtok(NULL, " \t\r\n"), 8);	// HTTP version
	
	// Test HTTP version
	resp.setVersion(&resp, version);
	if(supportedVersion(version)) {
		// Test supported method
		// Note: Whether method is allowed on the given resource is another story
		if(supportedMethod(key)) {
			// Try to find specified resource
			strcpy(file_path, rootPath);
			strcpy(&file_path[strlen(rootPath)],
					strncmp(value, "/\0" ,2) == 0 ? "/index.html" : value);
			int fd;
			if((fd=open(file_path, O_RDONLY)) != -1){
				char *Etag = NULL;
				char *modTimeStr = NULL;

				resp.statusCode = 200;
				resp.setBody(&resp, &fd, HEADER_BODY_TYPE_FILE); 
				resp.setField(&resp, "content-type", get_mime_type(file_path));

				// for line in data
				// If data read is empty line with CRLF only..
				// Then it is end of the header
				while((rcvd = recvLine(sockfd, buffer)) > 0 && strncmp(buffer, "\r\n", 2) != 0) {
					buffer[rcvd] = '\0';
					printf("%s", buffer); // Each line of valid header is guarantteed to end with CRLF
					char *strPtr = buffer;
					key = strsep(&strPtr, ":");
					value = strPtr+1;	// TODO: Check for malformed request
					
					if (handle_field(file_path, &resp, key, value)) break;
				}
				
				/*
				if(requireBody) {
					sendAll(sockfd, "HTTP/1.0 200 OK\r\n", 0);
					sendAll(sockfd, "content-type: ", 0);
					sendAll(sockfd, get_mime_type(file_path), 0);
					sendAll(sockfd, "\r\n", 0);
					
					if(modTimeStr) {
						sendAll(sockfd, "Last-Modified: ", 0);
						sendAll(sockfd, modTimeStr, 0);
						sendAll(sockfd, "\r\n", 0);
						free(modTimeStr);
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
				}
				*/
			} else {
				fprintf(stderr, "Cannot find specified resource: %s\n", file_path);
				resp.statusCode = 404;
			}
			
		} else {
			// Return error code 405 if method not supported..
			fprintf(stderr, "Unsupported method: %s\n", key);
			resp.statusCode = 405;
		}
	} else {
		// Return error code 505 if version not supported..
		fprintf(stderr, "Unsupported version: %s\n", version);
		resp.statusCode = 505;
	}
	
	char compiledStrBuffer[HEADER_COMPILE_MAX_LENGTH] = {0};
	size_t bytesCompiled = resp.compile(&resp, compiledStrBuffer);
	do {
		sendAll(sockfd, compiledStrBuffer, bytesCompiled);
    } while(bytesCompiled = resp.compile(NULL, compiledStrBuffer));
	
	shutdown(sockfd, SHUT_WR);
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
