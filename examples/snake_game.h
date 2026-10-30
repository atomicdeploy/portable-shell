#ifndef PORTABLE_SNAKE_DEMO_H
#define PORTABLE_SNAKE_DEMO_H
/* Byte-only demo model and retained renderer. Host owns keys, clock and terminal.
 * Viewport-sized arena; caller owns state and canvas buffers. */
#include "portable_screen.h"
#ifndef PS_SNAKE_CAPACITY
#define PS_SNAKE_CAPACITY 65536
#endif
#if PS_SNAKE_CAPACITY < 512
#error Snake capacity must cover the initial 32 by 16 arena
#endif
enum { SNAKE_WIDTH=32, SNAKE_HEIGHT=16, SNAKE_CAPACITY=PS_SNAKE_CAPACITY,
       SNAKE_UP=256,SNAKE_RIGHT,SNAKE_DOWN,SNAKE_LEFT };
typedef struct { unsigned x,y; } snake_point;
typedef struct {
    snake_point body[SNAKE_CAPACITY],food;
    unsigned length,direction,next,turned,score,best,paused,dead,won,quit;
    unsigned unicode,bell,esc,wrap,width,height;
    uint32_t random,last;
} snake_game;
static uint32_t snake_random(snake_game *g) {
    g->random=g->random*1664525u+1013904223u;return g->random;
}
static int snake_occupied(const snake_game *g,unsigned x,unsigned y,unsigned count) {
    unsigned i;for(i=0;i<count;++i)if(g->body[i].x==x && g->body[i].y==y)return 1;return 0;
}
static void snake_food(snake_game *g) {
    unsigned capacity=g->width*g->height,start=snake_random(g)%capacity,i,n;
    for(i=0;i<capacity;++i){
        n=(start+i)%capacity;
        if(!snake_occupied(g,n%g->width,n/g->width,g->length)){
            g->food.x=n%g->width;g->food.y=n/g->width;return;
        }
    }
    g->won=1;
}
static void snake_restart(snake_game *g,uint32_t now) {
    unsigned i;g->length=6;g->direction=g->next=1;g->turned=0;
    g->score=g->paused=g->dead=g->won=g->quit=g->esc=0;g->last=now;
    for(i=0;i<g->length;++i){g->body[i].x=g->width/2-4-i;g->body[i].y=g->height/2;}
    snake_food(g);
}
static void snake_init(snake_game *g,uint32_t now,unsigned unicode,unsigned bell) {
    g->width=SNAKE_WIDTH;g->height=SNAKE_HEIGHT;g->wrap=1;g->best=0;g->random=now^0x9123ab45u;g->unicode=unicode;g->bell=bell;snake_restart(g,now);
}
/* Preserve the round when it fits. A shrink that clips the snake starts a new
 * round, preserving best score, mute, pause and wrap preferences. */
static int snake_resize(snake_game *g,unsigned columns,unsigned rows,uint32_t now){
    unsigned width,height,i,reset=0,paused=g->paused;
    if(columns<80||rows<24||columns>512||rows>256)return 0;
    width=(columns-16)/2;height=rows-8;
    if(width==g->width&&height==g->height)return 1;
    if(width*height>SNAKE_CAPACITY)return 0;
    for(i=0;i<g->length;++i)if(g->body[i].x>=width||g->body[i].y>=height)reset=1;
    g->width=width;g->height=height;
    if(reset){snake_restart(g,now);g->paused=paused;}
    else if(g->food.x>=width||g->food.y>=height)snake_food(g);
    g->last=now;return 1;
}
static void snake_key(snake_game *g,unsigned key,uint32_t now) {
    unsigned d=4;
    if(key=='q'||key=='Q'||key==3){g->quit=1;return;}
    if(key=='m'||key=='M'){g->bell^=1;return;}
    if(key=='r'||key=='R'){snake_restart(g,now);return;}
    if(key==' '||key=='p'||key=='P'){g->paused^=1;g->last=now;return;}
    if(key==SNAKE_UP||key=='w'||key=='W')d=0;
    if(key==SNAKE_RIGHT||key=='d'||key=='D')d=1;
    if(key==SNAKE_DOWN||key=='s'||key=='S')d=2;
    if(key==SNAKE_LEFT||key=='a'||key=='A')d=3;
    if(d<4&&d!=g->direction&&!g->turned&&!g->paused&&!g->dead&&!g->won && d!=(g->direction+2)%4){
        g->next=d;g->turned=1;
    }
}
/* CSI and SS3 arrow sequences may arrive one byte at a time. */
static void snake_feed(snake_game *g,unsigned char byte,uint32_t now) {
    if(byte==27){g->esc=1;return;}
    if(g->esc==1){g->esc=(byte=='['||byte=='O')?2:0;return;}
    if(g->esc==2){
        if(byte>='0'&&byte<='9')return;
        if(byte==';')return;
        g->esc=0;
        if(byte=='A')snake_key(g,SNAKE_UP,now);
        if(byte=='B')snake_key(g,SNAKE_DOWN,now);
        if(byte=='C')snake_key(g,SNAKE_RIGHT,now);
        if(byte=='D')snake_key(g,SNAKE_LEFT,now);
        return;
    }
    snake_key(g,byte,now);
}
static unsigned snake_interval(const snake_game *g) {
    unsigned level=g->score/30;return level<12?180-level*10:60;
}
static void snake_bell(snake_game *g,ps_rich *out) {
    if(g->bell)ps_rich_raw(out,"\a",1);
}
static int snake_tick(snake_game *g,uint32_t now,ps_rich *out) {
    snake_point head;unsigned i,eating;
    if(g->paused||g->dead||g->won||g->quit){g->last=now;return 0;}
    if((uint32_t)(now-g->last)<snake_interval(g))return 0;
    /* Never replay a backlog of moves after a suspended host. */
    g->last=now;g->direction=g->next;g->turned=0;head=g->body[0];
    if(g->direction==0)--head.y;
    if(g->direction==1)++head.x;
    if(g->direction==2)++head.y;
    if(g->direction==3)--head.x;
    if(g->wrap){
        if(head.x>=g->width)head.x=g->direction==3?g->width-1:0;
        if(head.y>=g->height)head.y=g->direction==0?g->height-1:0;
    }
    eating=head.x==g->food.x && head.y==g->food.y;
    if(head.x>=g->width||head.y>=g->height||
       snake_occupied(g,head.x,head.y,g->length-(eating?0:1))){
        g->dead=1;snake_bell(g,out);return 1;
    }
    if(eating && g->length<SNAKE_CAPACITY)++g->length;
    for(i=g->length-1;i;--i)g->body[i]=g->body[i-1];
    g->body[0]=head;
    if(eating){g->score+=10;if(g->score>g->best)g->best=g->score;snake_food(g);snake_bell(g,out);}
    return 1;
}
static uint32_t snake_color(unsigned i) {
    unsigned sector=(i/6)%6,t=(i%6)*51;uint32_t rgb;
    switch(sector){
    case 0:rgb=0xff0000u|(t<<8);break;
    case 1:rgb=((255-t)<<16)|0x00ff00u;break;
    case 2:rgb=0x00ff00u|t;break;
    case 3:rgb=((255-t)<<8)|0xffu;break;
    case 4:rgb=(t<<16)|0x00ffu;break;
    default:rgb=0xff0000u|(255-t);break;
    }
    return rgb|0x303030u;
}
static void snake_number(ps_window *w,unsigned x,unsigned y,unsigned value,ps_cell c) {
    char text[12];unsigned n=0,i;do{text[n++]=(char)('0'+value%10);value/=10;}while(value);
    for(i=0;i<n/2;++i){char t=text[i];text[i]=text[n-1-i];text[n-1-i]=t;}
    text[n]=0;ps_window_text(w,x,y,text,c);
}
static void snake_render(snake_game *g,ps_screen *screen,ps_rich *out) {
    unsigned side=4+g->width*2;
    ps_window all=ps_screen_window(screen,0,0,screen->columns,screen->rows),arena=ps_screen_window(screen,1,3,g->width*2+2,g->height+2);
    ps_cell base=ps_cell_make(0xabb2bf,0x151923,0),muted=ps_cell_make(0x68748b,0x151923,0);
    ps_cell title=ps_cell_make(0x72e5de,0x151923,PS_TEXT_BOLD);
    ps_cell warm=ps_cell_make(0xffd166,0x151923,PS_TEXT_BOLD);
    unsigned i,x,y;const char *state=g->won?"COMPLETE":g->dead?"GAME OVER":g->paused?"PAUSED":"LIVE";
    ps_window_fill(&all,base);
    ps_window_text(&all,2,0,g->unicode?"◆  CHROMATIC / SNAKE":"*  CHROMATIC / SNAKE",title);
    char dimensions[64];ps_terminal_format(dimensions,sizeof dimensions,g->unicode?"%u × %u arena  %s":"%u x %u arena  %s",g->width,g->height,g->wrap?"WRAP":"WALLS");
    ps_window_text(&all,2,1,dimensions,muted);
    ps_window_text(&all,36,1,out->color==PS_COLOR_RGB?"RGB":out->color==PS_COLOR_256?"256 colors":out->color==PS_COLOR_16?"16 colors":"Monochrome",muted);
    ps_window_text(&all,screen->columns-12,0,state,g->dead?ps_cell_make(0xff657a,0x151923,1):warm);
    ps_window_border(&arena,ps_cell_make(g->dead?0xff657a:0x657899,0x151923,0),g->unicode);
    for(y=0;y<g->height;++y)for(x=0;x<g->width;++x)
        if((x+y)%2==0)ps_window_text(&all,2+x*2,4+y,g->unicode?"· ":". ",ps_cell_make(0x283244,0x151923,0));
    ps_window_text(&all,2+g->food.x*2,4+g->food.y,g->unicode?"◆ ":"* ",warm);
    for(i=g->length;i--;){
        uint32_t rgb=snake_color(i);
        ps_cell scale=ps_cell_make(rgb,((rgb>>2)&0x3f3f3fu),PS_TEXT_BOLD);
        const char *glyph=g->unicode?(i==g->length-1?"··":i%2?"╲╱":"╱╲"):"[]";
        if(!i)glyph=g->unicode?(g->direction==0?"▴▴":g->direction==1?"@›":g->direction==2?"▾▾":"‹@"):
                         (g->direction==0?"^^":g->direction==1?"o>":g->direction==2?"vv":"<o");
        ps_window_text(&all,2+g->body[i].x*2,4+g->body[i].y,glyph,scale);
    }
    ps_window_text(&all,side,4,"SCORE",muted);snake_number(&all,side,5,g->score,warm);
    ps_window_text(&all,side,7,"BEST",muted);snake_number(&all,side,8,g->best,title);
    ps_window_text(&all,side,10,"LENGTH",muted);snake_number(&all,side,11,g->length,base);
    ps_window_text(&all,side,13,"BELL",muted);ps_window_text(&all,side,14,g->bell?"ON":"OFF",g->bell?title:muted);
    ps_window_text(&all,side,17,g->unicode?"◆ +10":"* +10",warm);
    if(g->dead||g->paused||g->won){
        ps_cell banner=ps_cell_make(0xffffff,g->dead?0x96344c:0x285062,PS_TEXT_BOLD);
        ps_window_text(&all,2+g->width-14,4+g->height/2,g->dead?"  GAME OVER / R to restart  ":g->won?"  ARENA COMPLETE / R again  ":"     PAUSED / Space        ",banner);
    }
    ps_window_text(&all,2,screen->rows-2,g->unicode?"↑ ↓ ← → / WASD  Steer    Space  Pause    R  Restart    M  Bell    Q  Quit":
                                                        "WASD / Arrows  Steer    Space  Pause    R  Restart    M  Bell    Q  Quit",base);
    ps_window_text(&all,2,screen->rows-1,"Collect gems. Keep your tail. Every bite adds a new scale.",muted);
    ps_screen_present(screen,out);
}
#endif
