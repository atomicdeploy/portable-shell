#include "portable_shell.h"
#include "portable_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { COLUMNS=88, ROWS=22 };
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
unsigned ps_replay_background(unsigned,unsigned);
static char svg[800000];
static unsigned writes,now;
static void sink(void *context,const ps_byte *bytes,unsigned size){
    (void)context;ps_replay_feed(bytes,size);
}
static int read_value(void *context,unsigned id,ps_control_value *value){
    (void)context;memset(value,0,sizeof *value);value->flags=PS_CONTROL_VALID;
    if(id==2)value->flags|=PS_CONTROL_LOCKED;
    if(id==3)value->flags|=PS_CONTROL_PENDING;
    if(id==4)value->flags=0;
    if(id==1)strcpy(value->text,"Apply changes");
    if(id==2)strcpy(value->text,"Restart");
    if(id==3)strcpy(value->text,"Synchronize");
    return 1;
}
static int write_value(void *context,unsigned id,const ps_control_value *value,ps_u32 revision){
    (void)context;(void)value;(void)revision;assert(id<2);++writes;return 1;
}
static void draw(ps_session *s){ps_tick(s,now+=120);}
static unsigned find(const char *label,unsigned *column){
    unsigned y,x,i;
    for(y=0;y<ROWS;++y)for(x=0;x<COLUMNS;++x){
        for(i=0;label[i]&&x+i<COLUMNS;++i)
            if(ps_replay_cell(y,x+i)[0]!=label[i])break;
        if(!label[i]){*column=x;return y;}
    }
    assert(!"Expected rendered label");return 0;
}
static void capture(const char *path){
    FILE *f;unsigned n;if(!path)return;n=ps_replay_svg(svg);f=fopen(path,"wb");
    assert(f&&fwrite(svg,1,n,f)==n);assert(!fclose(f));
}
int main(int argc,char **argv){
    static const ps_control controls[]={
        {0,"","Refresh inventory",PS_CONTROL_ACTION,0,0,0,0,0},
        {1,"","Configuration",PS_CONTROL_ACTION,0,0,0,0,0},
        {2,"","Service restart",PS_CONTROL_ACTION,0,0,0,0,0},
        {3,"","Remote state",PS_CONTROL_ACTION,0,0,0,0,0},
        {4,"","Device discovery",PS_CONTROL_ACTION,0,0,0,0,0}};
    static const ps_control_page pages[]={{"Actions",controls,PS_COUNT_OF(controls)}};
    ps_panel panel={"Action controls","Demo provider",pages,1,read_value,write_value,0};
    ps_config config={sink,0,COLUMNS,ROWS,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE};
    void *memory=calloc(1,ps_sizeof_session());ps_session *s;
    unsigned x,y,idle,focused,disabled,pending,id;
    assert(memory);s=ps_init(memory,ps_sizeof_session(),&config);assert(s);
    ps_replay_reset();assert(ps_panel_attach(s,&panel)&&ps_panel_open(s,0));
    assert(ps_panel_select(s,1));draw(s);
    y=find("Run",&x);idle=ps_replay_background(y,x);
    assert(idle!=ps_replay_background(y,x+8)); /* bounded button background */
    y=find("Apply changes",&x);focused=ps_replay_background(y,x);
    assert(focused==ps_replay_background(y,x-1)); /* padding has the same fill */
    y=find("Restart",&x);disabled=ps_replay_background(y,x);
    y=find("Synchronize",&x);pending=ps_replay_background(y,x);
    assert(idle!=focused&&disabled!=focused&&pending!=idle);
    assert(writes==0);capture(argc>1?argv[1]:0);
    /* Disabled, pending and unknown buttons cannot execute. */
    for(id=2;id<5;++id){assert(ps_panel_select(s,id));ps_key(s,13);assert(ps_panel_selection(s,&x)&&x==id);}
    assert(writes==0);
    assert(ps_panel_select(s,1));draw(s);y=find("Apply changes",&x);
#if PS_MOUSE
    {char click[80];
     snprintf(click,sizeof click,"\033[<0;%u;%uM\033[<0;%u;%um",x+1,y+1,x+1,y+1);
     ps_feed(s,click,(unsigned)strlen(click));}
#else
    ps_key(s,13);
#endif
    draw(s);
    assert(!ps_panel_selection(s,&id)&&writes==0); /* activation opens confirmation */
    ps_key(s,13);assert(writes==0); /* Cancel is the initial dialog focus */
    ps_key(s,13);ps_key(s,9);ps_key(s,13);assert(writes==1);
    ps_resize(s,42,16);draw(s);assert(writes==1);
    ps_close(s);
    config.flags=PS_INTERACTIVE;s=ps_init(memory,ps_sizeof_session(),&config);assert(s);
    ps_replay_reset();assert(ps_panel_attach(s,&panel)&&ps_panel_open(s,0));draw(s);
    find("[ Run ]",&x);capture(argc>2?argv[2]:0);ps_close(s);free(memory);
    puts("Action buttons: distinct fills, bounded padding, custom labels, mouse confirmation, disabled guards and monochrome fallback passed");
    return 0;
}
