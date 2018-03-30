#include <stdio.h>
// For using pthread_tryjoin_np
#define _GNU_SOURCE
#include <pthread.h>
#include <poll.h>

#include "client.h"
#include "packet.h"

void *recv_and_display(void *arg);

int main(int argc, char *argv[]) {
	if(argc != 4) {
		printf("usage: end_point target_node_ip virtual_ip_self virtual_ip_tgt\n");
		return 0;
	}
	
	char *virtual_ip_self = argv[2];
	char *virtual_ip_target = argv[3];
    struct cscd58w18_client client = {0};
    client_init(&client, argv[1]);
    client.start(&client);
	
	// Boardcast existence
	struct cscd58w18_packet pkt_bcast = {0};
	pkt_bcast.ip_src = ip_to_int(virtual_ip_self);
	pkt_bcast.ip_dest = ip_to_int("255.255.255.255");
    client.send((struct cscd58w18_end_point *)&client, &pkt_bcast, CSCD58W18_PACKET_HEADER_SIZE);
	if(pkt_bcast.payload_length > 0) {
		client.send((struct cscd58w18_end_point *)&client, pkt_bcast.payload, pkt_bcast.payload_length);
	}
    
	
	// Router now knows where we are.. start listening for incoming packets
    pthread_t thread_listen;
    pthread_create(&thread_listen, NULL, recv_and_display, &client);
    
	char buffer[CSCD58W18_PACKET_PAYLOAD_LIMIT + 1] = {0};
	char *ptr = buffer;
	struct cscd58w18_packet pkt_data = {0};
	pkt_data.ip_src = ip_to_int(virtual_ip_self);
	pkt_data.ip_dest = ip_to_int(virtual_ip_target);
	pkt_data.hop_limit = 32;
	pkt_data.payload = buffer;
    int stdin_fd = fileno(stdin);
    while(1) {
		// Read until newline or 65535 bytes + '\0';
		fgets(ptr++, 2, stdin);
		
		if(*(ptr - 1) == '\n' || ptr - buffer >= CSCD58W18_PACKET_PAYLOAD_LIMIT) {
			if(*buffer == '@') {
				char *tmp = buffer + 1;
				char router_ip[INET6_ADDRSTRLEN] = {0};
				while(*tmp != ' ') ++tmp;
				strncpy(router_ip, buffer + 1, tmp - buffer - 1);
				
				
				
				struct cscd58w18_packet pkt_rt_ctrl = {0};
				pkt_rt_ctrl.ip_src = ip_to_int(virtual_ip_self);
				pkt_rt_ctrl.ip_dest = ip_to_int(router_ip);
				pkt_rt_ctrl.hop_limit = 32;
				pkt_rt_ctrl.payload_length = ptr - tmp - 1;
				pkt_rt_ctrl.payload = tmp + 1;
				pkt_rt_ctrl.flags_and_protocol |= CSCD58W18_PACKET_PROTOCOL_ROUTER_CTRL;
				client.send((struct cscd58w18_end_point *)&client, &pkt_rt_ctrl, CSCD58W18_PACKET_HEADER_SIZE);
				if(pkt_rt_ctrl.payload_length > 0) {
					client.send((struct cscd58w18_end_point *)&client, pkt_rt_ctrl.payload, pkt_data.payload_length);
				}
			} else {			
				pkt_data.payload_length = ptr - buffer;
				client.send((struct cscd58w18_end_point *)&client, &pkt_data, CSCD58W18_PACKET_HEADER_SIZE);
				if(pkt_data.payload_length > 0) {
					client.send((struct cscd58w18_end_point *)&client, pkt_data.payload, pkt_data.payload_length);
				}
			}
			ptr = buffer;
		}
    }
}

void *recv_and_display(void *arg) {
    struct cscd58w18_client *client = (struct cscd58w18_client *)arg;
    int rcvd = 0;
    int finished = 0;
	char source_ip[INET6_ADDRSTRLEN] = {0};
	char payload_buffer[CSCD58W18_PACKET_PAYLOAD_LIMIT + 1] = {0};
    struct cscd58w18_packet pkt_data = {0};
	pkt_data.payload = payload_buffer;
	
    while(!finished) {
		if((rcvd = client->recv((struct cscd58w18_end_point *)client, &pkt_data, CSCD58W18_PACKET_HEADER_SIZE)) > 0) {
			// Look at payload length
			if(pkt_data.payload_length > 0) {
				// Receive rest of the data
				rcvd = client->recv((struct cscd58w18_end_point *)client, pkt_data.payload, pkt_data.payload_length);
				((char *)pkt_data.payload)[pkt_data.payload_length] = '\0';
			}
			
			int_to_ip(pkt_data.ip_src, source_ip);
			printf("Received from: %s; bytes read: %d;\n", source_ip, rcvd);
			printf("Payload: %s\n", (char *)pkt_data.payload);
		}
        
        finished = rcvd <= 0;
    }
	
    printf("client: disconnected from remote\n");
    return NULL;
}
