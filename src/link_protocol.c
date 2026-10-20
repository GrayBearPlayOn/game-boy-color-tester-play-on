#include "link_protocol.h"
#include <string.h>
static uint16_t crc16(const uint8_t *p){
    uint8_t i,b;uint16_t c=0xffff;
    for(i=0;i<10;i++){c^=(uint16_t)p[i]<<8;for(b=0;b<8;b++)c=(c&0x8000)?(c<<1)^0x1021:c<<1;}return c;
}
static uint16_t word(const uint8_t *p){return p[0]|((uint16_t)p[1]<<8);}
void protocol_init(Protocol *p,uint8_t role){memset(p,0,sizeof(*p));p->role=role;}
void protocol_pack(const Protocol *p,uint8_t *d){
    uint16_t ack=p->have_rx?p->rx_seq:0xffff,c;
    d[0]=0x50;d[1]=0x4f;d[2]=1;d[3]=p->role;
    d[4]=p->tx_seq;d[5]=p->tx_seq>>8;d[6]=ack;d[7]=ack>>8;
    d[8]=d[4]^0x5a^d[3];d[9]=d[5]^0xa5;
    c=crc16(d);d[10]=c;d[11]=c>>8;
}
uint8_t protocol_receive(Protocol *p,const uint8_t *d){
    uint16_t seq=word(d+4),ack=word(d+6);uint8_t result=0;
    if(d[0]!=0x50||d[1]!=0x4f||d[2]!=1||d[3]!=(p->role^1)||word(d+10)!=crc16(d)||d[8]!=(d[4]^0x5a^d[3])||d[9]!=(d[5]^0xa5)){
        if(p->errors!=65535)p->errors++;return 0;
    }
    if(!p->have_rx){p->rx_seq=seq;p->have_rx=1;p->received++;result=1;}
    else if(seq==(uint16_t)(p->rx_seq+1)){p->rx_seq=seq;if(p->received!=65535)p->received++;result=1;}
    else if(seq!=p->rx_seq){if(p->errors!=65535)p->errors++;return 0;}
    /* A repeated packet is a valid retry; an ACK advances at most once. */
    if(ack==p->tx_seq&&p->sent){if(p->confirmed!=65535)p->confirmed++;p->tx_seq++;result|=2;}
    return result|4;
}
