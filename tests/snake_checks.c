#include "../examples/snake_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static unsigned bells,bytes;
static char svg[1200000];
static ps_cell cells[80*24],shown[80*24];
static void output(void *ctx,const unsigned char *s,size_t n){
    size_t i;(void)ctx;
    for(i=0;i<n;++i)if(s[i]==7)++bells;
    bytes+=(unsigned)n;ps_replay_feed(s,(unsigned)n);
}
int main(int argc,char **argv){
    snake_game g;ps_screen screen;ps_rich out={output,0,PS_COLOR_RGB,0,0};unsigned i,old;
    snake_init(&g,0,1,1);
    snake_key(&g,SNAKE_LEFT,0);assert(g.next==1);
    snake_feed(&g,27,0);snake_feed(&g,'[',0);snake_feed(&g,'A',0);
    snake_key(&g,SNAKE_LEFT,0);assert(g.next==0);
    assert(!snake_tick(&g,179,&out));assert(snake_tick(&g,180,&out));
    assert(g.body[0].y==7 && g.body[0].x==12);
    snake_key(&g,' ',181);assert(!snake_tick(&g,1000,&out));
    snake_key(&g,' ',1001);assert(!snake_tick(&g,1002,&out));
    snake_key(&g,'r',0);g.food.x=13;g.food.y=8;
    assert(snake_tick(&g,180,&out));assert(g.score==10&&g.length==7&&bells==1);
    assert(!snake_occupied(&g,g.food.x,g.food.y,g.length));
    g.body[0].x=31;assert(snake_tick(&g,360,&out)&&g.dead&&bells==2);
    snake_tick(&g,1000,&out);assert(bells==2);
    snake_key(&g,'r',0);assert(g.best==10&&!g.dead&&g.score==0);
    snake_key(&g,'m',0);g.body[0].x=31;snake_tick(&g,180,&out);assert(bells==2);
    snake_key(&g,'r',0);g.body[2].x=13;g.body[2].y=8;
    snake_tick(&g,180,&out);assert(g.dead);
    snake_restart(&g,UINT32_MAX-100);assert(snake_tick(&g,80,&out));
    /* Last free cell: deterministic full-arena victory without retry loops. */
    g.length=511;g.direction=g.next=3;g.body[0].x=1;g.body[0].y=0;
    for(i=1;i<511;++i){g.body[i].x=(i+1)%32;g.body[i].y=(i+1)/32;}
    g.food.x=0;g.food.y=0;g.last=0;g.dead=g.won=0;
    assert(snake_tick(&g,200,&out)&&g.won&&g.length==512);
    snake_key(&g,'q',201);assert(g.quit);
    /* Replay fixture: a longer curved snake, rendered by the real game renderer. */
    snake_init(&g,42,1,1);g.length=25;g.score=190;g.best=240;
    for(i=0;i<25;++i){
        if(i<10){g.body[i].x=23-i;g.body[i].y=5;}
        else if(i<16){g.body[i].x=14;g.body[i].y=5+i-9;}
        else {g.body[i].x=14-(i-15);g.body[i].y=11;}
    }
    g.food.x=27;g.food.y=5;
    ps_screen_init(&screen,cells,shown,80,24);ps_replay_reset();
    snake_render(&g,&screen,&out);old=bytes;snake_render(&g,&screen,&out);assert(old==bytes);
    if(argc>1){FILE *f=fopen(argv[1],"wb");unsigned n=ps_replay_svg(svg);assert(f);fwrite(svg,1,n,f);fclose(f);}
    g.unicode=0;out.color=PS_COLOR_NONE;snake_render(&g,&screen,&out);
    puts("Snake movement, paste-sized key bursts, collisions, victory, mute and BEL checks passed");
    return 0;
}
