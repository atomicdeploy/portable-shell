/* Actual emitted-byte replay with simulated values; no hardware claims. */
#include "portable_meter.h"
#include <stdio.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char svg[400000];
static void output(void *ctx,const unsigned char *p,size_t n){(void)ctx;ps_replay_feed(p,(unsigned)n);}
static void draw(ps_meter *m,ps_meter_style *s){ps_meter_render(m,s,output,0);ps_meter_emit(output,0,"\r\n");}
static int save(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");if(!f)return 3;
    if(fwrite(svg,1,n,f)!=n){fclose(f);return 4;}return fclose(f)?4:0;
}
int main(int argc,char **argv){
    ps_meter m={0,100,64,80,5,0,0};
    ps_meter_style s={72,PS_METER_LINE,PS_METER_TEXT_ABOVE,0,1,1,"Line / value above",0};
    int result;if(argc!=3)return 2;
    ps_replay_reset();draw(&m,&s);
    s.title="Line / value hidden";s.text=PS_METER_TEXT_NONE;draw(&m,&s);
    s.style=PS_METER_BLOCKS;s.text=PS_METER_TEXT_INSIDE;s.panel=1;
    s.title="Blocks / read-only / centered value";draw(&m,&s);
    s.title="Blocks / adjustable / paired markers";s.style|=PS_METER_ADJUST_MARKERS;
    m.adjustable=1;m.observed=80;draw(&m,&s);
    s.title="Blocks / adjustable / no markers";s.style=PS_METER_BLOCKS;draw(&m,&s);
    s.title="Blocks / observed value and independent target";s.style|=PS_METER_ADJUST_MARKERS;
    m.observed=64;m.show_requested=1;draw(&m,&s);
    s.title="Blocks / compact / value hidden";s.panel=0;s.text=PS_METER_TEXT_NONE;draw(&m,&s);
    result=save(argv[1]);if(result)return result;
    ps_replay_reset();
    s.style=PS_METER_TICKS;s.text=PS_METER_TEXT_ABOVE;s.title="Spaced ticks / value above";
    m.observed=57;m.show_requested=0;m.adjustable=0;draw(&m,&s);
    s.title="Spaced ticks / value hidden";s.text=PS_METER_TEXT_NONE;draw(&m,&s);
    s.style=PS_METER_CHEVRONS;s.panel=1;s.title="Chevron value / adjustable";
    s.subtitle="Observed readback and requested target are independent";
    m.observed=65;m.requested=80;m.adjustable=1;m.show_requested=1;draw(&m,&s);
    s.title="Chevron value / ASCII fallback";s.subtitle=0;s.unicode=0;m.show_requested=0;draw(&m,&s);
    s.title="Blocks / ASCII monochrome / adjustable";
    s.style=PS_METER_BLOCKS|PS_METER_ADJUST_MARKERS;s.text=PS_METER_TEXT_INSIDE;s.color=0;draw(&m,&s);
    s.title="Blocks / Unicode monochrome / read-only";s.unicode=1;m.adjustable=0;draw(&m,&s);
    return save(argv[2]);
}
