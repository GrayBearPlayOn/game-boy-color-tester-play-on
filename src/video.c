#include "app.h"
#include "assets.h"
#include "scenes.h"
#include <string.h>
#include <gb/isr.h>
static uint8_t framebuffer[2880];
static uint8_t sprite_tile[16];
static volatile uint8_t apa_mode;
static uint16_t offset(uint8_t x,uint8_t y);
static uint8_t dirty_left[18],dirty_right[18];
static uint8_t *copy_source;
static uint16_t copy_destination,copy_length;
static void unpack_scene(void) __naked {
    __asm
        push bc
        push de
        push hl
        ld a, (_copy_source)
        ld l, a
        ld a, (_copy_source+1)
        ld h, a
        ld de, #_framebuffer
scene_next:
        ld a, (hl+)
        or a
        jr Z, scene_done
        bit 7, a
        jr NZ, scene_repeat
        ld b, a
scene_literal:
        ld a, (hl+)
        ld (de), a
        inc de
        dec b
        jr NZ, scene_literal
        jr scene_next
scene_repeat:
        and #127
        inc a
        ld b, a
        ld a, (hl+)
scene_fill:
        ld (de), a
        inc de
        dec b
        jr NZ, scene_fill
        jr scene_next
scene_done:
        pop hl
        pop de
        pop bc
        ret
    __endasm;
}
void video_scene(uint8_t scene){
    copy_source=(uint8_t *)scenes[scene];unpack_scene();video_damage(0,0,160,144);
}
void video_damage(uint8_t x,uint8_t y,uint8_t w,uint8_t h){
    uint8_t row,last=(y+h-1)>>3,left=x>>3,right=(x+w-1)>>3;
    for(row=y>>3;row<=last;row++){
        if(left<dirty_left[row])dirty_left[row]=left;
        if(right+1>dirty_right[row])dirty_right[row]=right+1;
    }
}
void video_partial(void){memset(dirty_left,20,18);memset(dirty_right,0,18);}
void video_invert(uint8_t x,uint8_t y,uint8_t w,uint8_t h){
    uint8_t row,col,columns=w>>3,mask=0,*p;uint16_t at=offset(x,y);
    /* Selection bars start at x=0. Preserve the four unused edge pixels. */
    if(w&7)mask=255u<<(8-(w&7));
    for(row=0;row<h;row++){
        p=framebuffer+at;
        for(col=0;col<columns;col++){*p^=255;p+=8;}
        if(mask)*p^=mask;
        at+=((y+row)&7)==7?153:1;
    }
    video_damage(x,y,w,h);
}
static uint16_t offset(uint8_t x,uint8_t y){return (uint16_t)(y>>3)*160+(x>>3)*8+(y&7);}
void lcd_split(void) __critical __interrupt {}
static void map_frame(void) __naked {
    __asm
        push bc
        push de
        push hl
        ld hl, #0x9800
        ld de, #12
        ld c, #18
        xor a
map_row:
        ld b, #20
map_tile:
        ld (hl+), a
        inc a
        dec b
        jr NZ, map_tile
        add hl, de
        dec c
        jr NZ, map_row
        pop hl
        pop de
        pop bc
        ret
    __endasm;
}
static void map_attributes(void) __naked {
    __asm
        push bc
        push de
        push hl
        ld hl, #0x9800
        ld de, #0
        ld c, #18
attributes_row:
        ld b, #20
attributes_tile:
        ld a, e
        ld (hl+), a
        inc d
        jr NZ, attributes_same_bank
        ld e, #8
attributes_same_bank:
        dec b
        jr NZ, attributes_tile
        push de
        ld de, #12
        add hl, de
        pop de
        dec c
        jr NZ, attributes_row
        pop hl
        pop de
        pop bc
        ret
    __endasm;
}
static void copy_frame(void) __naked {
    __asm
        push bc
        push de
        push hl
        ld a, (_copy_source)
        ld l, a
        ld a, (_copy_source+1)
        ld h, a
        ld a, (_copy_destination)
        ld e, a
        ld a, (_copy_destination+1)
        ld d, a
        ld a, (_copy_length)
        ld c, a
        ld a, (_copy_length+1)
        ld b, a
        ldh a, (#0x40)
        bit 7, a
        jr Z, copy_frame_blank
copy_frame_wait:
        ldh a, (#0x41)
        and #0x02
        jr NZ, copy_frame_wait
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        dec bc
        ld a, b
        or c
        jr NZ, copy_frame_wait
        jr copy_frame_done
copy_frame_blank:
        srl b
        rr c
        srl b
        rr c
        srl b
        rr c
copy_frame_fast:
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld (de), a
        inc de
        dec bc
        ld a, b
        or c
        jr NZ, copy_frame_fast
copy_frame_done:
        pop hl
        pop de
        pop bc
        ret
    __endasm;
}
void video_clear(void){memset(framebuffer,0,sizeof(framebuffer));video_damage(0,0,160,144);}
void video_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t ink){
    uint8_t i,j,first,last,left,right;uint16_t at;
    if(!w||!h||x>=160||y>=144)return;
    if((uint16_t)x+w>160)w=160-x;if((uint16_t)y+h>144)h=144-y;
    first=x>>3;last=(x+w-1)>>3;left=255u>>(x&7);right=(x+w)&7;right=right?(255u<<(8-right)):255;
    at=offset(x,y);
    for(j=0;j<h;j++){
        if(first==last){i=left&right;if(ink)framebuffer[at]|=i;else framebuffer[at]&=~i;}
        else{
            if(ink)framebuffer[at]|=left;else framebuffer[at]&=~left;
            for(i=1;i<last-first;i++)framebuffer[at+(uint16_t)i*8]=ink?255:0;
            if(ink)framebuffer[at+(uint16_t)(last-first)*8]|=right;else framebuffer[at+(uint16_t)(last-first)*8]&=~right;
        }
        at+=((y+j)&7)==7?153:1;
    }
}
void video_box(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t ink){
    video_rect(x,y,w,1,ink);video_rect(x,y+h-1,w,1,ink);
    video_rect(x,y,1,h,ink);video_rect(x+w-1,y,1,h,ink);
}
void video_text(uint8_t x,uint8_t y,const char *s,uint8_t ink,uint8_t scale){
    uint16_t c,at;uint8_t g,r,col,shift,bits,part;
    while(*s){
        c=(uint8_t)*s++;
        if((c&0xe0)==0xc0)c=((c&31)<<6)|((uint8_t)*s++&63);
        if(x+5*scale>160)return;
        g=c<128?font_ascii[c]:(c==0x401?font_cyrillic[32]:(c>=0x410&&c<=0x42f?font_cyrillic[c-0x410]:0));
        if(scale==1){
            at=offset(x,y);shift=x&7;
            for(r=0;r<7;r++){
                bits=font_rows[g][r]<<3;part=bits>>shift;
                if(ink)framebuffer[at]|=part;else framebuffer[at]&=~part;
                if(shift>3){part=bits<<(8-shift);if(ink)framebuffer[at+8]|=part;else framebuffer[at+8]&=~part;}
                at+=((y+r)&7)==7?153:1;
            }
        }else for(r=0;r<7;r++)for(col=0;col<5;col++)if(font_rows[g][r]&(16>>col))video_rect(x+col*scale,y+r*scale,scale,scale,ink);
        x+=6*scale;
    }
}
void video_number(uint8_t x,uint8_t y,uint16_t value){
    char out[6];uint8_t n=5;out[5]=0;do{out[--n]='0'+value%10;value/=10;}while(value);video_text(x,y,out+n,1,1);
}
void video_hex(uint8_t x,uint8_t y,uint16_t v){
    char s[5];uint8_t i;const char *digits="0123456789ABCDEF";
    for(i=0;i<4;i++){s[3-i]=digits[v&15];v>>=4;}s[4]=0;video_text(x,y,s,1,1);
}
void video_logo(uint8_t left,uint8_t top){
    uint8_t x,y,bits,shift=left&7;uint16_t at=offset(left,top),src=0;
    for(y=0;y<80;y++){
        for(x=0;x<7;x++){bits=logo_mask[src++];framebuffer[at+(uint16_t)x*8]|=bits>>shift;if(shift)framebuffer[at+(uint16_t)(x+1)*8]|=bits<<(8-shift);}
        at+=((top+y)&7)==7?153:1;
    }
}
static const uint16_t gui_palette[]={RGB_WHITE,RGB_LIGHTGRAY,RGB_DARKGRAY,RGB_BLACK};
static const uint16_t fill_colors[]={RGB_WHITE,RGB_BLACK,RGB_RED,RGB_GREEN,RGB_BLUE,RGB_YELLOW,RGB_CYAN,RGB_PINK};
static void copy_region(uint16_t at,uint16_t length){
    uint16_t part;
    while(length){
        part=at<2048?2048-at:2880-at;if(part>length)part=length;
        VBK_REG=at>=2048;
        copy_source=framebuffer+at;copy_destination=0x8000+(at&2047)*2;
        copy_length=part;copy_frame();at+=part;length-=part;
    }
    VBK_REG=0;
}
void video_present(uint8_t reset){
    uint8_t y;uint16_t at;
    HIDE_SPRITES;SCX_REG=SCY_REG=0;
    if(reset||!apa_mode){
        DISPLAY_OFF;apa_mode=1;LCDC_REG=0x11;IE_REG&=~LCD_IFLAG;
        set_bkg_palette(0,1,gui_palette);set_sprite_palette(0,1,gui_palette);
        VBK_REG=0;map_frame();
        VBK_REG=1;map_attributes();
        copy_region(0,2880);LCDC_REG=0x91;
    }else for(y=0;y<18;y++)if(dirty_right[y]>dirty_left[y]){
        at=(uint16_t)y*160+dirty_left[y]*8;
        copy_region(at,(dirty_right[y]-dirty_left[y])*8);
    }
    app.tiles=360;
}
void video_gui(void){HIDE_SPRITES;SCX_REG=SCY_REG=0;}
void video_pattern(uint8_t kind,uint8_t variant){
    uint8_t tile[16],x,y,k,r,shade,bits,attr;uint16_t palette[32],color;
    DISPLAY_OFF;apa_mode=0;IE_REG&=~LCD_IFLAG;HIDE_SPRITES;LCDC_REG=0x11;SCX_REG=SCY_REG=0;VBK_REG=0;
    for(k=0;k<32;k++){
        if(kind==5)color=variant==0?RGB(k,k,k):(variant==1?RGB(k,0,0):(variant==2?RGB(0,k,0):RGB(0,0,k)));
        else color=gui_palette[k&3];
        palette[k]=color;
    }
    if(!kind)palette[0]=fill_colors[variant&7];
    set_bkg_palette(0,8,palette);set_sprite_palette(0,1,gui_palette);
    for(k=0;k<6;k++){
        for(r=0;r<8;r++){
            if(kind==5){tile[r*2]=0x0f;tile[r*2+1]=k==5?255:0;}
            else if(kind==2||kind==3){
                bits=kind==2?(variant?((r&7)?0x80:0xff):((r&1)?0x55:0xaa)):(variant?0xaa:((r&1)?0:0xff));
                tile[r*2]=tile[r*2+1]=bits;
            }else {tile[r*2]=(k&1)?255:0;tile[r*2+1]=(k&2)?255:0;}
        }
        if(kind==5&&k==0)memset(tile,0,16);
        set_bkg_data(k,1,tile);
    }
    for(y=0;y<18;y++)for(x=0;x<20;x++){
        attr=0;
        if(kind==5){shade=(x<2||x>=18)?0:4+((x-2)&1);if(x>=2&&x<18)attr=(x-2)>>1;}
        else shade=kind==1?(variant?y*4/18:x/5):0;
        VBK_REG=0;set_bkg_tile_xy(x,y,shade);VBK_REG=1;set_bkg_tile_xy(x,y,attr);
    }
    VBK_REG=0;
    if(kind==4){
        memset(sprite_tile,255,16);set_sprite_data(4,1,sprite_tile);
        for(k=0;k<16;k++){set_sprite_tile(k,4);set_sprite_prop(k,0);}
        SPRITES_8x8;SHOW_SPRITES;
    }
    LCDC_REG=kind==4?0x93:0x91;
}
void video_motion(uint8_t position,uint8_t reverse){
    uint8_t k,x=position;if(reverse)x=127-x;
    for(k=0;k<16;k++)move_sprite(k,x+(k&3)*8+8,56+(k>>2)*8+16);
}
