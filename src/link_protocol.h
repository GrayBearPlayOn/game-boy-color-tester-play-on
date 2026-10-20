#ifndef LINK_PROTOCOL_H
#define LINK_PROTOCOL_H
#include <stdint.h>
#define PACKET_SIZE 12
typedef struct {
    uint8_t role,have_rx;
    uint16_t tx_seq,rx_seq,sent,received,confirmed,errors;
} Protocol;
void protocol_init(Protocol *p,uint8_t role);
void protocol_pack(const Protocol *p,uint8_t *data);
uint8_t protocol_receive(Protocol *p,const uint8_t *data);
#endif
