#include "app.h"
static uint8_t melody_note;
static const uint8_t sine[16]={0x89,0xab,0xcd,0xde,0xee,0xdc,0xba,0x98,0x76,0x54,0x32,0x21,0x11,0x23,0x45,0x67};
static const uint16_t scale_notes[15]={262,294,330,349,392,440,494,523,494,440,392,349,330,294,262};
void audio_stop(void){NR51_REG=0;NR12_REG=NR22_REG=NR42_REG=0;NR30_REG=0;NR52_REG=0;}
void audio_start(uint8_t kind,uint16_t frequency,uint8_t route){
    uint8_t i,mask;uint16_t reg;
    audio_stop();NR52_REG=0x80;NR50_REG=0x33;
    if(kind==4)return; /* Powered, silent mixer: background-noise listening. */
    mask=1<<kind;NR51_REG=(route&1?mask<<4:0)|(route&2?mask:0);
    if(kind==2){
        NR30_REG=0;for(i=0;i<16;i++)_AUD3WAVERAM[i]=sine[i];
        NR31_REG=0;NR32_REG=0x60;NR30_REG=0x80;
        reg=2048u-(uint16_t)(65536ul/frequency);NR33_REG=reg;NR34_REG=0x80|(reg>>8);
    }else if(kind==0||kind==1){
        reg=2048u-(uint16_t)(131072ul/frequency);
        if(!kind){NR10_REG=0;NR11_REG=0x80;NR12_REG=0x40;NR13_REG=reg;NR14_REG=0x80|(reg>>8);}
        else {NR21_REG=0x80;NR22_REG=0x40;NR23_REG=reg;NR24_REG=0x80|(reg>>8);}
    }else{NR41_REG=0;NR42_REG=0x40;NR43_REG=0x45;NR44_REG=0x80;}
}
void audio_melody(void){melody_note=0;audio_start(2,scale_notes[0],3);}
uint8_t audio_update(uint16_t elapsed){
    uint8_t note=elapsed/20;if(note>=15){audio_stop();return 1;}
    if(note!=melody_note){melody_note=note;audio_start(2,scale_notes[note],3);}return 0;
}
