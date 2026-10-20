#include "app.h"
#include "link.h"
#include <gb/isr.h>
#include <string.h>
volatile LinkTest linktest;
static volatile uint8_t tx[PACKET_SIZE],tx_pos,queue[32],head,tail;
static uint8_t packet[PACKET_SIZE],fill,timeout_latched;
static uint16_t last_byte;
static void prepare(void){
    protocol_pack((Protocol *)&linktest.protocol,(uint8_t *)tx);
    if(linktest.protocol.sent!=65535)linktest.protocol.sent++;
}
void serial_isr(void) __critical __interrupt {
    uint8_t next=(head+1)&31;
    if(next!=tail){queue[head]=SB_REG;head=next;}
    if(++tx_pos==PACKET_SIZE){tx_pos=0;prepare();}
    if(linktest.active&&linktest.role){SB_REG=tx[tx_pos];SC_REG=0x80;}
}
void link_stop(void){linktest.active=0;SC_REG=0;IE_REG&=~SIO_IFLAG;}
void link_start(uint8_t mode,uint8_t role,uint16_t now){
    link_stop();memset((void *)&linktest,0,sizeof(linktest));
    protocol_init((Protocol *)&linktest.protocol,role);
    linktest.role=role;linktest.mode=mode;linktest.active=1;linktest.start=linktest.last_good=now;
    head=tail=tx_pos=fill=timeout_latched=0;last_byte=now;
    prepare();IF_REG&=~SIO_IFLAG;IE_REG|=SIO_IFLAG;
    if(role){SB_REG=tx[0];SC_REG=0x80;}
}
void link_poll(uint16_t now){
    uint8_t byte,result;
    if(!linktest.active)return;
    while(tail!=head){
        byte=queue[tail];tail=(tail+1)&31;last_byte=now;
        if(!fill){if(byte==0x50)packet[fill++]=byte;}
        else if(fill==1){if(byte==0x4f)packet[fill++]=byte;else fill=byte==0x50?1:0;}
        else {packet[fill++]=byte;
            if(fill==PACKET_SIZE){
                fill=0;CRITICAL{result=protocol_receive((Protocol *)&linktest.protocol,packet);}
                if(result&4){linktest.last_good=now;linktest.status=1;timeout_latched=0;}
            }
        }
    }
    /* One byte per video frame leaves ample time for the slave to re-arm. */
    if(!linktest.role&&!(SC_REG&0x80)){SB_REG=tx[tx_pos];SC_REG=0x81;}
    if((uint16_t)(now-linktest.last_good)>=120&&!timeout_latched){
        timeout_latched=1;linktest.timeouts++;linktest.status=2;
    }
    if((uint16_t)(now-last_byte)>=120){
        SC_REG=0;fill=tx_pos=0;last_byte=now;
        if(linktest.role){SB_REG=tx[0];SC_REG=0x80;}
    }
    if((linktest.mode==0&&linktest.protocol.confirmed>=1)||(linktest.mode==1&&linktest.protocol.confirmed>=100)){
        /* Keep responding after success so the partner can finish too. */
        linktest.status=3;
    }
}
