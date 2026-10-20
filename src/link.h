#ifndef LINK_H
#define LINK_H
#include "link_protocol.h"
typedef struct {
    Protocol protocol;
    uint8_t active,mode,status,role;
    uint16_t timeouts,last_good,start;
} LinkTest;
extern volatile LinkTest linktest;
void link_start(uint8_t mode,uint8_t role,uint16_t now);
void link_stop(void);
void link_poll(uint16_t now);
#endif
