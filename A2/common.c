#include "common.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int send_all(struct cscd58w18_end_point *this, void *buffer, size_t size) {
	int len = size > 0 ? size : strlen(buffer);
	int count = 0;
	int i = 0;
	
	// If not all bytes are sent, try to send the rest..
	do {
		// MSG_NOSIGNAL is important, if client broke up connection
		// Then send will yield Broken Pipe error
		// MSG_NOSIGNAL prevents broken pipe error cause program to halt
		i = send(this->sockfd, buffer + count, len - count, MSG_NOSIGNAL);
		count += i;
		if(i <= 0) {
			return i;
		}
	} while(len - i > 0);
	return len;
}

int recv_all(struct cscd58w18_end_point *this, void *buffer, size_t size) {
	int count = 0;
	int rcvd = 0;
	do {
		rcvd = recv(this->sockfd, buffer + count, size - count, 0);
		count += rcvd;
		if(rcvd <= 0) {
			return rcvd == 0 ? count : rcvd;
		}
	} while(size - count > 0);
	return count;
}

unsigned int ip_to_int(char *ip) {
	char *tmp = malloc(strlen(ip));
	strcpy(tmp, ip);
	char *ptr = tmp;
	int size = strlen(tmp);
	int ip_in_int = 0;
	int current_field = 3;
	
	int i = 0;
	for(i = 0; i <= size; ++i) {
		if(tmp[i] == '.' || tmp[i] == '\0') {
			tmp[i] = '\0';
			
			int number = atoi(ptr);
			printf("%d\n", number);
			ip_in_int |= number << (current_field-- * 8);
			ptr = tmp + i + 1;
		}
	}
	free(tmp);
	return ip_in_int;
}

int int_to_ip(unsigned int ip_in_int, char buffer[INET6_ADDRSTRLEN]) {
	unsigned int mask = 255 << 24;
	unsigned int field = 0;
	char *str = buffer;
	int written = 0;
	int count = 3;
	int i = 3;
	for(i = 3; i >= 0; --i) {
		field = (ip_in_int & mask) >> (i * 8);
		written = sprintf(str, "%d", field);
		str[written] = '.';
		str += written + 1;
		count += written;
		mask = mask >> 8;
	}
	str -= 1;
	str[0] = '\0';
	return count;
}
