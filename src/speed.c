#include "app.h"
volatile uint8_t cpu_result;
volatile uint16_t speed_loops;
void speed_set(uint8_t fast){
    if(fast)cpu_fast();else cpu_slow();
    cpu_result=(((KEY1_REG>>7)&1)==fast)?1:2;
}
void speed_probe(void){
    uint16_t start;
    vsync();CRITICAL{start=sys_time;}
    speed_loops=0;
    /* PPU/VBlank frequency stays constant across both CPU speeds. */
    while(sys_time==start)speed_loops++;
}
