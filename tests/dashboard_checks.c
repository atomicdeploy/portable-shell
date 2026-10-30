#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
static char output[262144],svg[800000];static unsigned used,revision=1,writes,clock_ms;
static unsigned values[]={0,88,80,0,40,80,0,1,0,0};
static union {void *alignment;unsigned char bytes[50000];} memory;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);
}
static int save(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;
    if(!f)return 1;
    bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
}
static int read_value(void *ctx,unsigned id,ps_control_value *v){
    (void)ctx;v->flags=PS_CONTROL_VALID;v->revision=revision;v->number=values[id];
    if(id==1)v->flags|=PS_CONTROL_LOCKED;
    if(id==3)strcpy(v->text,"Discharging");
    if(id==8)strcpy(v->text,"Source");
    if(id==9)strcpy(v->text,"Charger");
    return 1;
}
static int write_value(void *ctx,unsigned id,const ps_control_value *v,ps_u32 expected){
    (void)ctx;if(expected!=revision||id==1||id==3||id>=8)return 0;
    values[id]=v->number;++writes;++revision;return 1;
}
static void keys(ps_session *s,const char *text){
    used=0;ps_feed(s,text,(unsigned)strlen(text));ps_tick(s,clock_ms+=100);
}
int main(int argc,char **argv){
    const ps_control charge[]={{1,"","Charge level",PS_CONTROL_LEVEL,0,100,1,0,0},{2,"","Charge limit",PS_CONTROL_LEVEL,0,100,5,0,0},{3,"","State",PS_CONTROL_LABEL,0,0,0,0,0}};
    const ps_control brightness[]={{4,"","Fingerprint",PS_CONTROL_LEVEL,0,100,5,0,0},{5,"","Keyboard",PS_CONTROL_LEVEL,0,100,5,0,0}};
    const ps_control privacy[]={{6,"","Microphone",PS_CONTROL_TOGGLE,0,1,1,0,0},{7,"","Camera",PS_CONTROL_TOGGLE,0,1,1,0,0}};
    const ps_control ports[]={{8,"","Left port",PS_CONTROL_LABEL,0,0,0,0,0},{9,"","Right port",PS_CONTROL_LABEL,0,0,0,0,0}};
    const ps_control_page pages[]={{"Charge",charge,3},{"Brightness",brightness,2},{"Privacy",privacy,2},{"Ports",ports,2}};
    const ps_panel panel={"System controls","Simulated provider | Revision-checked writes",pages,4,read_value,write_value,0};
    ps_config cfg={sink,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_TRUECOLOR};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    assert(ps_panel_attach(s,&panel));
#if PS_DASHBOARD
    assert(ps_panel_set_layout(s,PS_PANEL_GRID)&&ps_panel_open(s,0));
    keys(s,"");assert(strstr(output,"Charge")&&strstr(output,"Brightness")&&strstr(output,"Privacy")&&strstr(output,"Ports"));
    keys(s,"\033[C");assert(!writes&&values[1]==88);
    keys(s,"\033[B\033[C");assert(writes==1&&values[2]==85);
    keys(s,"\t\033[C");assert(values[4]==45);
    keys(s,"\r\02535");assert(values[4]==45);keys(s,"\r");assert(values[4]==35);
    keys(s,"\r\025999\r");assert(values[4]==35);ps_key(s,27);
    keys(s,"\t ");assert(values[6]==1);
    keys(s,"\033[Z\033[F");assert(values[4]==100);keys(s,"\033[C");assert(values[4]==100);
#if PS_MOUSE
    keys(s,"\033[<0;5;13M ");assert(values[6]==0);
#endif
    ps_resize(s,60,24);keys(s,"\t");ps_resize(s,96,24);
    keys(s,"\033");ps_key(s,27);assert(ps_panel_set_layout(s,PS_PANEL_TABS));
    {
        ps_control_page more[5];ps_panel extended=panel;unsigned i;
        for(i=0;i<4;++i)more[i]=pages[i];
        more[4].title="Extra panel";more[4].controls=0;more[4].count=0;
        extended.pages=more;extended.count=5;
        assert(ps_panel_attach(s,&extended)&&ps_panel_set_layout(s,PS_PANEL_GRID));
        assert(ps_panel_open(s,4));keys(s,"");assert(strstr(output,"Extra panel"));
        keys(s,"\t");assert(strstr(output,"Charge"));
        ps_key(s,27);assert(ps_panel_attach(s,&panel));
    }
#else
    (void)keys;assert(!ps_panel_set_layout(s,PS_PANEL_GRID));assert(ps_panel_set_layout(s,PS_PANEL_TABS));
#endif
    if(argc==1)return 0;
    if(argc!=3)return 2;
    values[4]=40;values[6]=0;ps_panel_set_layout(s,PS_PANEL_GRID);ps_replay_reset();used=0;
    ps_panel_open(s,0);keys(s,"\033[B");ps_resize(s,96,24);
    if(save(argv[1]))return 3;
    keys(s,"\t\r\02565");ps_resize(s,96,24);return save(argv[2]);
}
