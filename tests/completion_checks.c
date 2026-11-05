#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
unsigned ps_replay_row(void);
unsigned ps_replay_column(void);
unsigned ps_replay_background(unsigned,unsigned);
const char *ps_replay_cell(unsigned,unsigned);
enum { COLUMNS=80, ROWS=12, SVG_CAPACITY=400000 };
static char svg[SVG_CAPACITY],output[131072];
static unsigned used;
static void sink(void *context,const ps_byte *bytes,unsigned count){
    (void)context;assert(used+count<sizeof output);
    memcpy(output+used,bytes,count);used+=count;output[used]=0;ps_replay_feed(bytes,count);
}
static int command(ps_session *s,int argc,const char *const *argv,void *context){
    (void)s;(void)argc;(void)argv;(void)context;return 0;
}
static void feed(ps_session *s,const char *text){ps_feed(s,text,(unsigned)strlen(text));}
static int locate(const char *text,unsigned *y,unsigned *x){
    for(*y=0;*y<ROWS;++*y)for(*x=0;*x<COLUMNS;++*x){
        unsigned column=*x;const char *p=text;
        while(*p&&column<COLUMNS){const char *cell=ps_replay_cell(*y,column++);size_t n=strlen(cell);
            if(!n||strncmp(p,cell,n))break;
            p+=n;
        }
        if(!*p)return 1;
    }return 0;
}
static void selected(const char *name){
    unsigned y,x;assert(locate(name,&y,&x));
    assert(ps_replay_background(y,x)!=ps_replay_background(y,COLUMNS-1));
}
static void capture(const char *path){
    FILE *f;unsigned n;if(!path)return;n=ps_replay_svg(svg);assert(n<sizeof svg);
    f=fopen(path,"wb");assert(f);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));
}
int main(int argc,char **argv){
    ps_command commands[]={
        {"connect","","",command,0,0},{"configure","","",command,0,0},
        {"console","","",command,0,0}
    };
    unsigned mode,i;void *memory=malloc(ps_sizeof_session());assert(memory);
    for(mode=0;mode<3;++mode){
        ps_config cfg={sink,0,COLUMNS,ROWS,PS_INTERACTIVE|PS_NO_SUGGESTIONS};ps_session *s;
        if(!mode)cfg.flags|=PS_COLOR|PS_TRUECOLOR|PS_UNICODE;
        if(mode==2)cfg.flags|=PS_NO_COMPLETION_MENU;
        used=0;ps_replay_reset();s=ps_init(memory,ps_sizeof_session(),&cfg);assert(s);
        for(i=0;i<PS_COUNT_OF(commands);++i)assert(!ps_register(s,&commands[i]));
        assert(!ps_set_prompt(s,"workbench\n> ",PS_STYLE_COMMAND));ps_start(s);
        feed(s,"con\t");
        feed(s,"\t");
        if(PS_COMPLETION_MENU&&mode!=2){selected("connect");assert(ps_replay_column()==9);}
        feed(s,"\t");
        if(PS_COMPLETION_MENU&&mode!=2){selected("configure");assert(ps_replay_column()==11);}
        if(!mode)capture(argc>1?argv[1]:0);
        feed(s,"\t\t");
        if(PS_COMPLETION_MENU&&mode!=2)selected("connect");
        ps_key(s,PS_KEY_BACKTAB);
        if(PS_COMPLETION_MENU&&mode!=2)selected("console");
        feed(s,"\r");assert(!strcmp(ps_history_get(s,0),"console"));
        ps_key(s,12);feed(s,"con\t\t");ps_resize(s,12,6);ps_key(s,9);
        if(PS_COMPLETION_MENU&&mode!=2){selected("configure");assert(ps_replay_column()<12);}
        ps_key(s,PS_KEY_HOME);feed(s,"x\r");
        assert(!strcmp(ps_history_get(s,1),"xconfigure"));
        ps_resize(s,COLUMNS,ROWS);ps_key(s,12);feed(s,"con\t\t");ps_key(s,27);feed(s,"\r");
        assert(!strcmp(ps_history_get(s,2),"connect"));
        ps_close(s);
    }
    free(memory);puts("Completion selection, forward/reverse rollover, resize, dismissal and monochrome passed");return 0;
}
