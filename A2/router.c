#include <stdio.h>

#include "server.h"
#include "packet.h"

// Routing entry must be sorted by range_bits, in ascending order..
// Where most specific (least range_bits) are at front
struct routing_entry {
    // How many bits are ranges, starting at right
    // e.g. 00000001.00000000.00000000.********
    // Has 8 range bits to the right
    // Mapping 1.0.0.*
    int ip;
    int port; // Actually a sockfd
    signed char range_bits;
    struct routing_entry* next;
};

struct routing_entry* rtable = NULL;
void rtable_insert(int ip, int port, signed char range);
struct routing_entry *rtable_get_route(int ip);
int rtable_size();
char *rtable_dump();


int router_listen_thread(struct cscd58w18_server *server, int connection_fd);


char *virtual_ip = NULL;
int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("usage: router actual_ip virtual_ip\n");
		return 0;
	}
    
	virtual_ip = argv[2];
    struct cscd58w18_server server = {0};
    server_init(&server, argv[1]);
	server.connection_handler = router_listen_thread;
    server.start(&server);
    
	//router_listen_thread(&server, connection_fd);
	while(1) {}
}

void rtable_insert(int ip, int port, signed char range) {
	// Create new entry
	struct routing_entry *new_entry = malloc(sizeof(struct routing_entry));
	new_entry->ip = ip;
    new_entry->port = port;
    new_entry->range_bits = range;
	new_entry->next = NULL;
	
	// Insertion
	// If rtable does not exist, initialize it
	if(rtable) {
		// Find a suitable spot..
		struct routing_entry *it_ptr_prev = rtable;
		struct routing_entry *it_ptr = rtable;
		
		// Find suitable slot
		while(it_ptr) {
			if(it_ptr->range_bits >= range) {
				break;
			}
			it_ptr_prev = it_ptr;
			it_ptr = it_ptr->next;
		}
		
		while(it_ptr && it_ptr->range_bits == range) {
			if(it_ptr->ip >= ip) {
				break;
			}
			it_ptr_prev = it_ptr;
			it_ptr = it_ptr->next;
		}
		
		// Already have this ip in table
		if(it_ptr_prev->range_bits == range && it_ptr_prev->ip == ip) {
			// Update entry
			it_ptr_prev->port = new_entry->port;
		}
		
		if(it_ptr == rtable) {
			new_entry->next = rtable;
			rtable = new_entry;
		} else {
			new_entry->next = it_ptr;
			it_ptr_prev->next = new_entry;
		}
	} else {
		rtable = new_entry;
	}
};

struct routing_entry *rtable_get_route(int ip) {
	struct routing_entry *it_ptr = rtable;
	while(it_ptr) {
		uint32_t mask = (~0) << it_ptr->range_bits;
		if((it_ptr->ip & mask) == (ip & mask)) break;
		it_ptr = it_ptr->next;
	}
	return it_ptr;
}

int rtable_size() {
	struct routing_entry *it_ptr = rtable;
	int count = 0;
	while(it_ptr) {
		++count; it_ptr = it_ptr->next; 
	}
	return count;
}

char *rtable_dump() {
	struct routing_entry *it_ptr = rtable;
	int size = rtable_size();
	// For each line
	// 16 char for IP + 3 space + 5 char for port + 4 space + 2 char for range bits count + \r\n
	// total = 32 bytes per line
	char *dump_buffer = malloc(size * 32 + 1);
	char *buffer_ptr = dump_buffer;
	memset(dump_buffer, ' ', size * 32);
	dump_buffer[size * 32] = '\0';
	while(it_ptr) {
		int written = int_to_ip(it_ptr->ip, buffer_ptr);
		buffer_ptr[written] = ' ';
		written = sprintf(buffer_ptr + 16 + 3, "%d", it_ptr->port);
		buffer_ptr[16 + 3 + written] = ' ';
		written = sprintf(buffer_ptr + 16 + 3 + 5 + 4, "%d", it_ptr->range_bits);
		buffer_ptr[16 + 3 + 5 + 4 +written] = ' ';
		buffer_ptr[30] = '\r';
		buffer_ptr[31] = '\n';
		
		buffer_ptr += 32;
		it_ptr = it_ptr->next;
	}
	return dump_buffer;
}

int router_listen_thread(struct cscd58w18_server *server, int connection_fd) {
    // while(not terminate) 
    //      recv()
    //      if(success recv)
    //          forward table look up
    //          TTL -= 1
    //          send(ip, packet)
    //
    int finished = 0;
    int rcvd = 0;
	int server_ip_int = ip_to_int(server->ip);
    
    struct cscd58w18_end_point node = {0};
    node.sockfd = connection_fd;
    
	// Receive broadcasted info
    struct cscd58w18_packet pkt_bcast = {0};
	rcvd = server->recv(&node, &pkt_bcast, CSCD58W18_PACKET_HEADER_SIZE);
	
	// Insert to routing table
	// port param we passed in is actually a sockfd
	rtable_insert(pkt_bcast.ip_src, connection_fd, 0);
    
	char ip_str[INET6_ADDRSTRLEN] = {0};
	char payload_buffer[CSCD58W18_PACKET_PAYLOAD_LIMIT + 1] = {0};
    struct cscd58w18_packet pkt_data = {0};
	pkt_data.payload = payload_buffer;
    while(!finished) {
        //struct cscd58w18_packet packet = {0};
        //rcvd = server->recv(this, &packet, sizeof(struct cscd58w18_packet));
        
        if((rcvd = server->recv(&node, &pkt_data, CSCD58W18_PACKET_HEADER_SIZE)) > 0) {	
			// Look at payload length
			if(pkt_data.payload_length > 0) {
				// Receive rest of the data
				rcvd = server->recv(&node, pkt_data.payload, pkt_data.payload_length);
				((char *)pkt_data.payload)[pkt_data.payload_length] = '\0';
			}
			
			int_to_ip(pkt_data.ip_src, ip_str);
			printf("Received from: %s; bytes read: %d;\n", ip_str, rcvd);
			printf("Payload: %s\n", (char *)pkt_data.payload);
			
			struct cscd58w18_end_point outgoing_node = {0};
			if(pkt_data.ip_dest == (~0)) {
				// Is this a broadcast?
				rtable_insert(pkt_data.ip_src, connection_fd, 0);
				// For each broadcast, rtable is updated..
				// Need advertise to neighbor routers
				
			} else if(pkt_data.ip_dest == ip_to_int(virtual_ip) && PACKET_PROTOCOL(&pkt_data) == CSCD58W18_PACKET_PROTOCOL_ROUTER_CTRL) {
				// This is a command sent to this server
				if(strncmp("print_forwarding_table", pkt_data.payload, 22) == 0) {
					char * dump = rtable_dump();
					printf("print_forwarding_table\nIP Address      PORT  range_bits\n%s\n", dump);
					free(dump);
				} else if(strncmp("get_forwarding_table", pkt_data.payload, 20) == 0) {
					char * dump = rtable_dump();
					pkt_data.payload_length = strlen(dump) < CSCD58W18_PACKET_PAYLOAD_LIMIT ? strlen(dump): CSCD58W18_PACKET_PAYLOAD_LIMIT;
					memcpy(pkt_data.payload, dump, pkt_data.payload_length);
					printf("Replying to: %s\n", ip_str);
					outgoing_node.sockfd = connection_fd; // Send to whoever asked for it
					free(dump);
				//} else if(strncmp("set_forwarding_table", pkt_data->payload, 20) == 0) {
				//} else if(strncmp("advertise", pkt_data->payload, 20) == 0) {
				//} else if(strncmp("update", pkt_data->payload, 20) == 0) {
				} else {
					printf("Message for router: %s\n", (char *)pkt_data.payload);
				}
			} else {
				pkt_data.hop_limit -= 1;
				if(pkt_data.hop_limit >= 0) {
					// Check routing table
					struct routing_entry *r = rtable_get_route(pkt_data.ip_dest);
					outgoing_node.sockfd = r->port; // Actually a socket fd
					//outgoing_node.sockfd = connection_fd; // Loop-back for debugging
					int_to_ip(r->ip, ip_str);
					printf("Forwarding to: %s\n", ip_str);
				} else {
					printf("Hop limit exceeded, packet dropped\n");
				}
			}

			server->send(&outgoing_node, &pkt_data, CSCD58W18_PACKET_HEADER_SIZE);
			if(pkt_data.payload_length > 0) {
				server->send(&outgoing_node, pkt_data.payload, pkt_data.payload_length);
			}
		}
		
        finished = rcvd <= 0;
    }
    
	close(connection_fd);
    printf("server: connection terminated\n");
}