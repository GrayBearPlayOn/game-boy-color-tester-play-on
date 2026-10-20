"""Rasterize the fixed UI at build time, using the original pixel font and logo.

Coordinates and text here are the canonical static screen layout. Runtime code
only draws changing values; no emulator or image library is needed to build.
"""
import json

HOME=['КНОПКИ','ЗВУК','ЭКРАН','НАУШНИКИ','КАБЕЛЬ СВЯЗИ','ПРОВЕРКА ПАМЯТИ','GBC ТЕСТЫ','О ПРОГРАММЕ']
SOUND=['ТЕСТОВАЯ ГАММА','НИЗКИЙ ТОН','СРЕДНИЙ ТОН','ВЫСОКИЙ ТОН','КАНАЛЫ 1-4','ФОНОВЫЙ ШУМ']
SCREEN=['ЦВЕТНЫЕ ЗАЛИВКИ','ЧЕТЫРЕ ОТТЕНКА','СЕТКА / ШАХМАТКА','ЛИНИИ','ДВИЖЕНИЕ','АВТОСМЕНА']
HEADPHONES=['ЛЕВЫЙ КАНАЛ','ПРАВЫЙ КАНАЛ','ЧЕРЕДОВАНИЕ Л/П','ФОНОВЫЙ ШУМ']
APU=['КАНАЛ 1: ИМПУЛЬС','КАНАЛ 2: ИМПУЛЬС','КАНАЛ 3: ВОЛНА','КАНАЛ 4: ШУМ']
LINK=['ПОИСК ПАРТНЕРА','ПРОВЕРКА ОБМЕНА','ДЛИТЕЛЬНЫЙ ТЕСТ']

class Canvas:
    def __init__(self,font,logo):self.data=bytearray(2880);self.font=font;self.logo=logo
    def pixel(self,x,y,ink=1):
        if not(0<=x<160 and 0<=y<144):return
        at=(y//8)*160+(x//8)*8+y%8;mask=128>>(x%8)
        if ink:self.data[at]|=mask
        else:self.data[at]&=255^mask
    def rect(self,x,y,w,h,ink=1):
        for yy in range(y,y+h):
            for xx in range(x,x+w):self.pixel(xx,yy,ink)
    def box(self,x,y,w,h):
        self.rect(x,y,w,1);self.rect(x,y+h-1,w,1);self.rect(x,y,1,h);self.rect(x+w-1,y,1,h)
    def text(self,x,y,text,scale=1,ink=1):
        for ch in text:
            if x+5*scale>160:return
            for row,bits in enumerate(self.font.get(ord(ch),[0]*7)):
                for col in range(5):
                    if bits&(16>>col):self.rect(x+col*scale,y+row*scale,scale,scale,ink)
            x+=6*scale
    def brand(self,x,y):
        for yy,row in enumerate(self.logo):
            for xx,p in enumerate(row):
                if p=='1':self.pixel(x+xx,y+yy)
    def title(self,s):self.text(6,7,s);self.rect(4,24,152,1)
    def footer(self,s):self.rect(4,124,152,1);self.text(6,133,s)
    def listing(self,items,spacing=12):
        for i,item in enumerate(items):self.text(6,32+i*spacing,item)
    def button(self,x,y,w,h,s):
        self.box(x,y,w,h)
        if s:self.text(x+(w-5)//2,y+(h-7)//2,s)

def packed(data):
    """PackBits: 1..127 literals, 128..255 repeated byte, zero terminator."""
    result=[];pos=0
    while pos<len(data):
        n=1
        while pos+n<len(data) and data[pos+n]==data[pos] and n<128:n+=1
        if n>=3:result.extend((127+n,data[pos]));pos+=n;continue
        start=pos;pos+=1
        while pos<len(data) and pos-start<127:
            if pos+2<len(data) and data[pos]==data[pos+1]==data[pos+2]:break
            pos+=1
        result.append(pos-start);result.extend(data[start:pos])
    result.append(0);return result

def generate(root):
    font={g['code']:g['rows'] for g in json.loads((root/'assets/font.json').read_text(encoding='utf8'))}
    logo=json.loads((root/'assets/logo.json').read_text(encoding='utf8'))['rows']
    scenes=[]
    def new():
        c=Canvas(font,logo);scenes.append(c);return c
    c=new();c.text(6,5,'ТЕСТ GB',2);c.text(136,9,'GBC');c.rect(4,24,152,1)
    c.listing(HOME,11);c.brand(100,33);c.rect(4,120,152,1);c.text(6,131,'\1\2 ВЫБОР  A ОТКРЫТЬ')
    c=new();c.title('ТЕСТ КНОПОК');c.text(90,7,'ВЫХОД: 2 С');c.text(84,16,'START+SELECT')
    for args in [(31,44,18,18,'\1'),(31,88,18,18,'\2'),(9,66,18,18,'\3'),(53,66,18,18,'\4'),(126,48,22,22,'A'),(101,75,22,22,'B'),(72,105,23,9,''),(112,105,23,9,'')]:c.button(*args)
    c.box(36,71,8,8);c.text(64,116,'SELECT');c.text(109,116,'START');c.rect(4,127,152,1);c.text(6,134,'ПРОВЕРЕНО:');c.text(72,134,'0');c.text(80,134,'/8')
    for title,items in [('ЗВУК',SOUND),('ЭКРАН',SCREEN),('НАУШНИКИ',HEADPHONES)]:
        c=new();c.title(title);c.listing(items)
        c.footer('B НАЗАД  A ОТКРЫТЬ' if title=='ЭКРАН' else 'B НАЗАД  A ПУСК/СТОП')
        if title=='НАУШНИКИ':c.text(6,109,'ОЦЕНИВАЙТЕ НА СЛУХ')
    c=new();c.title('КАБЕЛЬ СВЯЗИ');c.listing(LINK);c.text(6,78,'РОЛЬ:');c.text(6,92,'\3\4 СМЕНА РОЛИ');c.text(6,108,'2 КОНСОЛИ + КАБЕЛЬ');c.footer('B НАЗАД  A ПУСК')
    c=new();c.title('ПРОВЕРКА ПАМЯТИ') # dynamic memory contents
    c=new();c.title('О ПРОГРАММЕ')
    c.text(6,36,'Программа для');c.text(6,48,'тестирования');c.text(6,60,'Gameboy Color')
    c.text(6,78,'https://vk.ru/play0n');c.text(6,94,'https://t.me/bear_play_on')
    c.footer('B НАЗАД')
    c=new();c.title('ЗВУКОВЫЕ КАНАЛЫ');c.listing(APU);c.footer('B НАЗАД  A ПУСК/СТОП')
    c=new();c.rect(4,24,152,1);c.text(6,38,'ТЕСТ ЗАЙМЕТ ВЕСЬ ЭКРАН');c.text(6,56,'\3\4 МЕНЯЮТ ВАРИАНТ');c.text(6,74,'A АВТОСМЕНА ЗАЛИВОК');c.text(6,92,'ДЕФЕКТЫ ОЦЕНИВАЙТЕ');c.text(6,104,'ВИЗУАЛЬНО');c.footer('B НАЗАД  A ОТКРЫТЬ')
    c=new();c.title('КАБЕЛЬ СВЯЗИ');c.text(6,63,'ОТПРАВЛЕНО:');c.text(6,74,'ПРИНЯТО:');c.text(6,85,'ПОДТВЕРЖДЕНО:');c.text(6,100,'ОШИБКИ:');c.text(6,112,'ТАЙМ-АУТ:');c.footer('B НАЗАД  A СТОП')
    c=new();c.title('GBC ТЕСТЫ');c.listing(['ЦВЕТОВЫЕ ШКАЛЫ','СКОРОСТЬ CPU','БАНКИ ПАМЯТИ']);c.footer('B НАЗАД  A ОТКРЫТЬ')
    c=new();c.title('СКОРОСТЬ CPU');c.listing(['ОБЫЧНАЯ: 4 МГЦ','УДВОЕННАЯ: 8 МГЦ']);c.text(6,72,'ТЕКУЩАЯ:');c.text(6,102,'B ВОЗВРАЩАЕТ 4 МГЦ');c.footer('B НАЗАД  A ВКЛЮЧИТЬ')
    out=['/* Generated static UI; layout source: scripts/ui_assets.py. */']
    for i,c in enumerate(scenes):
        data=packed(c.data);out.append(f'static const unsigned char scene_{i}[]={{'+','.join(map(str,data))+'};')
    out.append('static const unsigned char *const scenes[]={'+','.join(f'scene_{i}' for i in range(len(scenes)))+'};')
    (root/'build/scenes.h').write_text('\n'.join(out)+'\n',encoding='ascii')
