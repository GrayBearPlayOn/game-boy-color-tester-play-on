#include "app.h"
#include "link.h"
#include <string.h>
volatile Application app;
static uint16_t repeat_start;
static uint8_t last_direction,headphone_side,link_role,ignore_b;
static volatile uint8_t irq_held,irq_pressed,irq_released;
static volatile uint8_t irq_combo_active,irq_exit;
static volatile uint16_t irq_combo_start;
static uint8_t released_frame;
static void input_vblank(void){
    uint8_t held=joypad();
    irq_pressed|=held&~irq_held;irq_released|=irq_held&~held;irq_held=held;
    if((held&(J_START|J_SELECT))==(J_START|J_SELECT)){
        if(!irq_combo_active){irq_combo_active=1;irq_combo_start=sys_time;}
        if(app.view==BUTTONS&&(uint16_t)(sys_time-irq_combo_start)>=120)irq_exit=1;
    }else irq_combo_active=0;
}
static const uint8_t views[]={BUTTONS,SOUND,SCREEN,HEADPHONES,LINK,MEMORY,CGB_TESTS,ABOUT};
static uint8_t count(uint8_t value){uint8_t n=0;while(value){n+=value&1;value>>=1;}return n;}
static void label(uint8_t x,uint8_t y,const char *s){video_text(x,y,s,1,1);}
static void footer(const char *s){video_rect(4,124,152,1,1);label(6,133,s);}
static void button(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t key,const char *s){
    uint8_t held=(app.held&key)!=0;
    video_box(x,y,w,h,1);if(held)video_rect(x+1,y+1,w-2,h-2,1);
    if(app.checked&key){video_rect(x+w-4,y+2,2,2,!held);video_box(x+2,y+2,w-4,h-4,1);}
    if(*s)video_text(x+(w-5)/2,y+(h-7)/2,s,!held,1);
}
static void draw_link(void){
    if(!linktest.active){
        label(42,78,link_role?"ВЕДОМАЯ":"ВЕДУЩАЯ");
    }else{
        label(6,32,linktest.role?"ВЕДОМАЯ":"ВЕДУЩАЯ");
        label(6,46,linktest.status==3?(linktest.protocol.errors?"ОБМЕН: ЕСТЬ ОШИБКИ":"ПРОВЕРКА ПРОЙДЕНА"):(linktest.status==2?"НЕТ ОТВЕТА":(linktest.status==1?"ИДЕТ ОБМЕН":"ПОИСК ПАРТНЕРА")));
        video_number(112,63,linktest.protocol.sent);
        video_number(112,74,linktest.protocol.received);
        video_number(112,85,linktest.protocol.confirmed);
        video_number(112,100,linktest.protocol.errors);
        video_number(112,112,linktest.timeouts);
    }
}
static void draw_memory(void){
    if(!memtest.running&&!memtest.done&&!memtest.cancelled){
        label(6,34,"WRAM: 7 БАНКОВ / 28 КБ");label(6,48,"VRAM: 2 БАНКА / 16 КБ");
        label(6,68,"ПРИ ПРОВЕРКЕ VRAM");label(6,80,"ЭКРАН БУДЕТ СВЕТЛЫМ");label(6,102,"СОХРАНЕНИЯ НЕ ЗАТРОНУТЫ");
    }else{
        label(6,32,memtest.running?(memtest.region?"ОБЛАСТЬ: VRAM":"ОБЛАСТЬ: WRAM"):(memtest.done?"ПРОВЕРКА ЗАВЕРШЕНА":"ОТМЕНЕНО"));
        if(memtest.running){label(96,32,"B");video_number(108,32,memtest.walk);}
        label(6,47,"ЭТАП:");video_number(48,47,memtest.phase<12?memtest.phase+1:12);label(66,47,"/12");
        label(6,61,"ПРОВЕРЕНО:");video_number(78,61,memtest.checked);label(114,61,"БАЙТ");
        label(6,76,"ОШИБКИ:");video_number(78,76,memtest.errors);
        if(memtest.errors){label(130,91,"B");video_number(142,91,memtest.first_bank);label(6,91,"АДРЕС:");video_hex(78,91,memtest.first);label(6,104,"ЖДАЛ:");video_hex(42,104,memtest.expected);label(78,104,"ЧТ:");video_hex(102,104,memtest.actual);}
        else if(memtest.done)label(6,101,"ОШИБОК НЕ ОБНАРУЖЕНО");
    }
    footer(memtest.running?"B ОТМЕНА":"B НАЗАД  A ПУСК");
}
static uint8_t drawn_selection,drawn_held,drawn_checked,drawn_playing,drawn_automatic,drawn_role,drawn_link_active;
static void update_button(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t key,const char *s){
    if(((drawn_held^app.held)|(drawn_checked^app.checked))&key){
        video_rect(x,y,w,h,0);button(x,y,w,h,key,s);video_damage(x,y,w,h);
    }
}
static void draw_speed(void){
    label(66,72,(KEY1_REG&0x80)?"8 МГЦ":"4 МГЦ");
    label(6,86,cpu_result==2?"ПЕРЕКЛЮЧЕНИЕ: ОШИБКА":"ПЕРЕКЛЮЧЕНИЕ: OK");
    label(6,114,"ЦИКЛОВ/КАДР:");video_number(90,114,speed_loops);
}
static uint8_t incremental(void){
    uint8_t width=app.view==HOME?96:156,y;
    video_partial();
    switch(app.view){
    case LINK:
        if(linktest.active||drawn_link_active)return 0;
        if(drawn_role!=link_role){
            video_rect(42,78,42,7,0);label(42,78,link_role?"ВЕДОМАЯ":"ВЕДУЩАЯ");video_damage(42,78,42,7);
        }
        /* Inactive Link has the same cursor as the other menus. */
        goto menu_cursor;
    case BUTTONS:
        update_button(31,44,18,18,J_UP,"\1");update_button(31,88,18,18,J_DOWN,"\2");
        update_button(9,66,18,18,J_LEFT,"\3");update_button(53,66,18,18,J_RIGHT,"\4");
        update_button(126,48,22,22,J_A,"A");update_button(101,75,22,22,J_B,"B");
        update_button(72,105,23,9,J_SELECT,"");update_button(112,105,23,9,J_START,"");
        if(drawn_checked!=app.checked){video_rect(72,134,6,7,0);video_number(72,134,count(app.checked));video_damage(72,134,6,7);}
        return 1;
    case HOME:case SOUND:case APU:case HEADPHONES:case SCREEN:case CGB_TESTS:
menu_cursor:
        if(drawn_selection!=app.selection){
            video_invert(0,30+drawn_selection*(app.view==HOME?11:12),width,app.view==HOME?10:11);
            video_invert(0,30+app.selection*(app.view==HOME?11:12),width,app.view==HOME?10:11);
        }
        if((app.view==SOUND||app.view==APU||app.view==HEADPHONES)&&drawn_playing!=app.playing){
            y=app.view==SOUND?111:(app.view==APU?100:95);
            video_rect(6,y,72,7,0);label(6,y,app.playing?"ИГРАЕТ":"ОСТАНОВЛЕН");video_damage(6,y,72,7);
        }
        if(app.view==SCREEN&&drawn_automatic!=app.automatic){
            video_rect(6,112,84,7,0);label(6,112,app.automatic?"АВТОСМЕНА: ДА":"АВТОСМЕНА: НЕТ");video_damage(6,112,84,7);
        }
        return 1;
    }
    return 0;
}
static void render(void){
    uint8_t reset=app.redraw==2;
    if(reset||!incremental()){
        video_scene(app.view==SCREEN_HELP?9:(app.view==LINK&&linktest.active?10:app.view));
        drawn_selection=app.selection;drawn_held=drawn_checked=0;
        drawn_playing=drawn_automatic=255;
        if(app.view==HOME||app.view==SOUND||app.view==APU||app.view==HEADPHONES||app.view==SCREEN||app.view==CGB_TESTS||app.view==SPEED||(app.view==LINK&&!linktest.active))
            video_invert(0,30+app.selection*(app.view==HOME?11:12),app.view==HOME?96:156,app.view==HOME?10:11);
        if(app.view==SPEED)draw_speed();
        else if(app.view==SCREEN_HELP)label(6,7,app.screen_kind==5?"ЦВЕТОВЫЕ ШКАЛЫ":(app.screen_kind==0?"ЦВЕТНЫЕ ЗАЛИВКИ":(app.screen_kind==1?"ЧЕТЫРЕ ОТТЕНКА":(app.screen_kind==2?"СЕТКА / ШАХМАТКА":(app.screen_kind==3?"ЛИНИИ":"ДВИЖЕНИЕ")))));
        else if(app.view==LINK)draw_link();
        else if(app.view==MEMORY)draw_memory();
        else incremental();
        video_damage(0,0,160,144);
    }
    video_present(reset);app.redraw=0;
    drawn_selection=app.selection;drawn_held=app.held;drawn_checked=app.checked;
    drawn_playing=app.playing;drawn_automatic=app.automatic;
    drawn_role=link_role;drawn_link_active=linktest.active;
}
static void enter(uint8_t view,uint8_t held){
    if(app.view==SPEED&&view!=SPEED)speed_set(0);
    audio_stop();app.playing=0;link_stop();HIDE_SPRITES;
    app.view=view;app.selection=0;app.redraw=2;app.exit_hold=0;
    if(view==HOME)app.selection=app.home;
    if(view==BUTTONS){app.checked=app.armed=0;app.previous=held;irq_combo_active=irq_exit=0;}
    if(view==MEMORY)memset((void *)&memtest,0,sizeof(memtest));
    video_gui();
}
static void toggle_sound(uint16_t now){
    if(app.playing){audio_stop();app.playing=0;app.redraw=1;return;}
    app.playing=1;app.audio_start=now;headphone_side=0;app.redraw=1;
    if(app.view==SOUND){
        if(!app.selection)audio_melody();
        else if(app.selection==5)audio_start(4,1000,3);
        else audio_start(2,app.selection==1?250:(app.selection==2?1000:4000),3);
    }else if(app.view==APU)audio_start(app.selection,1000,3);
    else audio_start(app.selection==3?4:2,1000,app.selection==1?2:1);
}
static void inputs(uint8_t held,uint8_t pressed,uint8_t repeat,uint16_t now){
    uint8_t n,selection;
    if(app.view==BUTTONS){
        uint8_t released=released_frame,old=app.checked;
        if(irq_exit){irq_exit=0;enter(HOME,held);return;}
        app.armed|=pressed;app.checked|=released&app.armed;app.armed&=~released;
        if(held!=app.previous||old!=app.checked)app.redraw=1;
        if((held&(J_START|J_SELECT))==(J_START|J_SELECT)){
            app.exit_hold=(uint16_t)(now-irq_combo_start)>120?120:now-irq_combo_start;
        }else app.exit_hold=0;
        return;
    }
    if(app.view==FULLSCREEN){
        if(pressed&J_B){selection=app.screen_kind;enter(selection==5?CGB_TESTS:SCREEN,held);app.selection=selection==5?0:selection;return;}
        if((pressed&J_A)&&!app.screen_kind){app.automatic^=1;app.screen_start=now;}
        if(pressed&(J_LEFT|J_RIGHT)){
            app.variant=app.screen_kind==5?((app.variant+((pressed&J_RIGHT)?1:3))&3):(app.screen_kind?(app.variant^1):((app.variant+((pressed&J_RIGHT)?1:7))&7));
            video_pattern(app.screen_kind,app.variant);app.screen_start=now;
        }
        return;
    }
    if(app.view==SCREEN_HELP){
        if(pressed&J_B){selection=app.screen_kind;enter(selection==5?CGB_TESTS:SCREEN,held);app.selection=selection==5?0:selection;}
        else if(pressed&J_A){app.view=FULLSCREEN;app.redraw=0;app.screen_start=now;video_pattern(app.screen_kind,app.variant);}
        return;
    }
    if(app.view==MEMORY){
        if(pressed&J_B){if(memtest.running){memory_cancel();app.redraw=1;}else enter(HOME,held);}
        else if((pressed&J_A)&&!memtest.running){memory_start();app.redraw=2;}
        return;
    }
    if((pressed&J_B)&&app.view!=HOME){if(app.view==APU)enter(SOUND,held);else if(app.view==SPEED)enter(CGB_TESTS,held);else enter(HOME,held);return;}
    if(app.view==ABOUT)return;
    if(app.view==LINK&&linktest.active){if(pressed&J_A){link_stop();app.redraw=1;}return;}
    n=app.view==HOME?8:((app.view==SOUND||app.view==SCREEN)?6:((app.view==LINK||app.view==CGB_TESTS)?3:(app.view==SPEED?2:4)));
    if(repeat&(J_UP|J_DOWN)){
        app.selection=(app.selection+((repeat&J_DOWN)?1:n-1))%n;
        audio_stop();app.playing=0;app.redraw=1;
    }
    if(app.view==LINK&&(pressed&(J_LEFT|J_RIGHT))){link_role^=1;app.redraw=1;}
    if(!(pressed&J_A))return;
    if(app.view==HOME){app.home=app.selection;enter(views[app.selection],held);}
    else if(app.view==CGB_TESTS){
        if(app.selection==0){app.screen_kind=5;app.variant=0;enter(SCREEN_HELP,held);}
        else enter(app.selection==1?SPEED:MEMORY,held);
    }
    else if(app.view==SPEED){speed_set(app.selection);speed_probe();app.redraw=1;}
    else if(app.view==SOUND&&app.selection==4)enter(APU,held);
    else if(app.view==SOUND||app.view==HEADPHONES||app.view==APU)toggle_sound(now);
    else if(app.view==SCREEN){
        if(app.selection==5){app.automatic^=1;app.redraw=1;}
        else {app.screen_kind=app.selection;app.variant=0;enter(SCREEN_HELP,held);}
    }else if(app.view==LINK){link_start(app.selection,link_role,now);app.redraw=1;}
}
void main(void){
    uint8_t held,pressed,repeat,dir,side;uint16_t now,elapsed;
    DISPLAY_OFF;SVBK_REG=1;VBK_REG=0;speed_set(0);audio_stop();SC_REG=0;LCDC_REG=0x11;BGP_REG=0xe4;OBP0_REG=0xe4;OBP1_REG=0xe4;
    app.held=app.previous=irq_held=joypad();add_VBL(input_vblank);app.redraw=2;set_interrupts(VBL_IFLAG);enable_interrupts();render();
    while(1){
        vsync();CRITICAL{now=sys_time;}app.frames=now;
        CRITICAL{held=irq_held;pressed=irq_pressed;released_frame=irq_released;irq_pressed=irq_released=0;}
        if(ignore_b){pressed&=~J_B;if(!(held&J_B))ignore_b=0;}
        repeat=pressed;app.held=held;
        dir=held&(J_UP|J_DOWN);
        if(dir&&dir==last_direction){if((uint16_t)(now-repeat_start)>=25){repeat|=dir;repeat_start=now-18;}}
        else {last_direction=dir;repeat_start=now;}
        inputs(held,pressed,repeat,now);
        if(app.playing){
            elapsed=now-app.audio_start;
            if(app.view==SOUND&&!app.selection&&audio_update(elapsed)){app.playing=0;app.redraw=1;}
            if(app.view==HEADPHONES&&app.selection==2){side=(elapsed/60)&1;if(side!=headphone_side){headphone_side=side;audio_start(2,1000,side?2:1);}}
        }
        if(app.view==LINK&&linktest.active){link_poll(now);if(!(now&15))app.redraw=1;}
        if(app.view==MEMORY&&memtest.running){memory_step();if(memtest.cancelled&&memtest.region)ignore_b=1;}
        if(app.view==FULLSCREEN){
            if(!app.screen_kind&&app.automatic&&(uint16_t)(now-app.screen_start)>=180){app.variant=(app.variant+1)&7;app.screen_start=now;video_pattern(0,app.variant);}
            if(app.screen_kind==4)video_motion(((now-app.screen_start)*2)%128,app.variant);
        }else if(app.redraw)render();
        app.previous=held;
    }
}
