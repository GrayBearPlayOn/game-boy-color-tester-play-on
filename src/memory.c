#include "app.h"
#include <string.h>
volatile MemoryTest memtest;
/* Globals and stack are in fixed C000-CFFF. No stack lives in banked WRAM. */
static uint8_t pattern(uint16_t address,uint8_t phase){
    switch(phase){case 0:return 0;case 1:return 255;case 2:return 0xaa;case 3:return 0x55;case 4:return (uint8_t)address^(address>>8);default:return ~((uint8_t)address^(address>>8));}
}
static void failure(uint16_t a,uint8_t expected,uint8_t actual){
    if(!memtest.errors){memtest.first=a;memtest.expected=expected;memtest.actual=actual;memtest.first_bank=memtest.walk;}
    if(memtest.errors!=65535)memtest.errors++;
}
void memory_cancel(void){memtest.running=0;memtest.cancelled=1;}
void memory_start(void){
    uint8_t bank,value,actual,bit;
    memset((void *)&memtest,0,sizeof(memtest));
    memtest.running=1;memtest.total=45056;memtest.walk=1;
    /* Different signatures must coexist: sequential per-bank tests alone
       would miss a stuck bank selector that aliases all banks together. */
    for(bank=1;bank<=7;bank++){SVBK_REG=bank;*(volatile uint8_t *)0xd000=0x39^bank;}
    for(bank=1;bank<=7;bank++){
        SVBK_REG=bank;memtest.walk=bank;actual=*(volatile uint8_t *)0xd000;
        if(actual!=(0x39^bank))failure(0xd000,0x39^bank,actual);
        for(bit=0;bit<8;bit++){
            value=1<<bit;*(volatile uint8_t *)0xd001=value;actual=*(volatile uint8_t *)0xd001;if(actual!=value)failure(0xd001,value,actual);
            value=~value;*(volatile uint8_t *)0xd001=value;actual=*(volatile uint8_t *)0xd001;if(actual!=value)failure(0xd001,value,actual);
        }
    }
    /* VRAM bank alias check while LCD is off. Rendering restores both banks. */
    DISPLAY_OFF;
    VBK_REG=0;*(volatile uint8_t *)0x8000=0x35;
    VBK_REG=1;*(volatile uint8_t *)0x8000=0xca;
    VBK_REG=0;actual=*(volatile uint8_t *)0x8000;
    if(actual!=0x35){memtest.walk=0;failure(0x8000,0x35,actual);}
    VBK_REG=1;actual=*(volatile uint8_t *)0x8000;
    if(actual!=0xca){memtest.walk=1;failure(0x8000,0xca,actual);}
    SVBK_REG=1;VBK_REG=0;memtest.walk=1;app.redraw=2;
}
void memory_step(void){
    uint16_t base,size,end,a;uint8_t v,actual;
    volatile uint8_t *p;
    if(!memtest.running)return;
    base=memtest.region?0x8000:0xd000;size=memtest.region?8192:4096;
    if(memtest.region){
        DISPLAY_OFF;
        while(memtest.running){
            VBK_REG=memtest.walk;
            end=memtest.index+64;if(end>size)end=size;
            for(a=memtest.index;a<end;a++){
                p=(volatile uint8_t *)(base+a);v=pattern(base+a,memtest.phase>>1);
                if(!(memtest.phase&1))*p=v;else {actual=*p;if(actual!=v)failure(base+a,v,actual);}
            }
            memtest.index=end;
            if(joypad()&J_B){memory_cancel();break;}
            if(end==size){
                memtest.index=0;
                if(++memtest.phase==12){
                    memtest.checked+=size;memtest.phase=0;
                    if(memtest.walk==0)memtest.walk=1;
                    else {memtest.phase=12;memtest.running=0;memtest.done=1;}
                }
            }
        }
        VBK_REG=0;SVBK_REG=1;app.redraw=2;return;
    }
    SVBK_REG=memtest.walk;
    end=memtest.index+256;if(end>size)end=size;
    for(a=memtest.index;a<end;a++){
        p=(volatile uint8_t *)(base+a);v=pattern(base+a,memtest.phase>>1);
        if(!(memtest.phase&1))*p=v;else {actual=*p;if(actual!=v)failure(base+a,v,actual);}
    }
    SVBK_REG=1;memtest.index=end;
    if(end==size){
        memtest.index=0;app.redraw=1;
        if(++memtest.phase==12){
            memtest.checked+=size;memtest.phase=0;
            if(++memtest.walk==8){memtest.region=1;memtest.walk=0;}
        }
    }
}
