#ifndef _CSCD58W18_PACKET_H_
#define _CSCD58W18_PACKET_H_

#include <stdint.h>
#include "common.h"


// 2x32 bits IP for origin and destination
// 16 bits field for body length
// 8 bits hop limit
// 4 bits Flags
// 4 bits Protocol number
// 4 bytes reserved
// Total: 8 + 2 + 1 + 1 + 4 = 16 bytes for header
// Variable length body
// size(header) = 16 bytes
// size(packet) = size(header + body) = 16 bytes + at_most 65536 bytes

// Protocol number
// 2^4 = 16 possible protocols
// 0 - Data exchange
// 1 - router control
// 2 - router broadcast OSPF
// 3 - router broadcast RIP

#define CSCD58W18_PACKET_HEADER_SIZE 16
#define CSCD58W18_PACKET_PAYLOAD_LIMIT 65536
#define CSCD58W18_PACKET_PROTOCOL_ROUTER_CTRL	1
#define CSCD58W18_PACKET_PROTOCOL_ROUTER_OSPF	2
#define CSCD58W18_PACKET_PROTOCOL_ROUTER_RIP	3
#define CSCD58W18_PACKET_MASK_FLAGS				240
#define CSCD58W18_PACKET_MASK_PROTOCOL			15
#define PACKET_FLAGS(pkt)  ((pkt)->flags_and_protocol & CSCD58W18_PACKET_MASK_FLAGS)
#define PACKET_PROTOCOL(pkt)  ((pkt)->flags_and_protocol & CSCD58W18_PACKET_MASK_PROTOCOL)

// uint32_t guarantees 32 bits integer
struct cscd58w18_packet {
    uint32_t ip_src;
    uint32_t ip_dest;
    uint16_t payload_length;
    char hop_limit;
    char flags_and_protocol;
	char reserved[4];
    void *payload;
};

// Buffer must have enough size to hold all data..
// Use packet_get_size to find out how big the buffer should be
// Returns a pointer to buffer (same pointer that user passed in)
char * packet_serialize(struct cscd58w18_packet *pkt, char *buffer);
// Returns the size of packet, includes header and body
int packet_get_size(struct cscd58w18_packet *pkt);

#endif
