#include "packet.h"


char * packet_serialize(struct cscd58w18_packet *pkt, char *buffer) {
	memcpy((void *)buffer, (void *)pkt, 16);
	if(pkt->payload_length > 0) memcpy((void *)(buffer + 16), pkt->payload, pkt->payload_length);
	return buffer;
}

int packet_get_size(struct cscd58w18_packet *pkt) {
	return CSCD58W18_PACKET_HEADER_SIZE + pkt->payload_length;
}