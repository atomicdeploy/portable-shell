/* Replay fixture: renderer output, simulated observations; no hardware claims. */
#include "portable_meter.h"
#include <stdio.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char svg[400000];
static void output(void *ctx,const unsigned char *p,size_t n){(void)ctx;ps_replay_feed(p,(unsigned)n);}
int main(int argc,char **argv){
    ps_meter m={0,100,64,80,5,0,0};
    ps_meter_style s={72,PS_METER_LINE,PS_METER_TEXT_ABOVE,0,1,1,"Transfer","Read-only progress"};
    FILE *f;unsigned n;
    if(argc!=2)return 2;
    ps_replay_reset();ps_meter_render(&m,&s,output,0);
    ps_meter_emit(output,0,"\r\n");
    s.style=PS_METER_BLOCKS;s.text=PS_METER_TEXT_INSIDE;s.panel=1;
    s.title="Output level";s.subtitle="Adjustable target";m.adjustable=1;
    m.observed=m.requested;ps_meter_render(&m,&s,output,0);
    ps_meter_emit(output,0,"\r\n");
    s.title="Temperature controller";s.subtitle="Live observation with an independent target";
    m.observed=64;m.show_requested=1;ps_meter_render(&m,&s,output,0);
    ps_meter_emit(output,0,"\r\n");
    s.title="Compact";s.subtitle=0;s.panel=0;s.text=PS_METER_TEXT_NONE;
    ps_meter_render(&m,&s,output,0);
    n=ps_replay_svg(svg);f=fopen(argv[1],"wb");if(!f)return 3;
    if(fwrite(svg,1,n,f)!=n){fclose(f);return 4;}return fclose(f)!=0;
}
