#ifndef APP_H
#define APP_H
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
enum {HOME,BUTTONS,SOUND,SCREEN,HEADPHONES,LINK,MEMORY,ABOUT,APU,SCREEN_HELP,FULLSCREEN,CGB_TESTS,SPEED};
typedef struct {
    uint8_t view,selection,home,held,previous,checked,armed,exit_hold;
    uint8_t playing,screen_kind,variant,automatic,redraw,video_error;
    uint16_t frames,audio_start,screen_start,tiles;
} Application;
extern volatile Application app;
extern volatile uint8_t cpu_result;
extern volatile uint16_t speed_loops;
void speed_set(uint8_t fast);
void speed_probe(void);
void video_clear(void);
void video_scene(uint8_t scene);
void video_partial(void);
void video_damage(uint8_t x,uint8_t y,uint8_t w,uint8_t h);
void video_invert(uint8_t x,uint8_t y,uint8_t w,uint8_t h);
void video_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t ink);
void video_box(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t ink);
void video_text(uint8_t x,uint8_t y,const char *text,uint8_t ink,uint8_t scale);
void video_number(uint8_t x,uint8_t y,uint16_t value);
void video_hex(uint8_t x,uint8_t y,uint16_t value);
void video_logo(uint8_t x,uint8_t y);
void video_present(uint8_t reset);
void video_pattern(uint8_t kind,uint8_t variant);
void video_motion(uint8_t position,uint8_t reverse);
void video_gui(void);
void audio_stop(void);
void audio_start(uint8_t kind,uint16_t frequency,uint8_t route);
void audio_melody(void);
uint8_t audio_update(uint16_t elapsed);
typedef struct {
    uint8_t running,region,phase,done,cancelled,walk;
    uint16_t index,errors,first,expected,actual,checked,total;
    uint8_t first_bank;
} MemoryTest;
extern volatile MemoryTest memtest;
void memory_start(void);
void memory_step(void);
void memory_cancel(void);
#endif
