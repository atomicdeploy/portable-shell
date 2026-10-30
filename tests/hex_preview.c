/* Deterministic preview from real editor output; no filesystem in the core. */
#include "portable_shell.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
static union {void *align;ps_byte bytes[PS_DEFAULT_SESSION_STORAGE];} memory;
static char svg[1200000];
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;ps_replay_feed(p,n);}
static int save(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;
    if(!f)return 1;
    bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
}
int main(int argc,char **argv){
    ps_byte source[1024],draft[1024];unsigned i;
    ps_config config={sink,0,96,28,PS_INTERACTIVE|PS_COLOR|PS_UNICODE};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&config);
    if((argc!=1&&argc!=3)||!s)return 2;
    for(i=0;i<sizeof source;++i)source[i]=(ps_byte)i;
    ps_replay_reset();
    if(!ps_hex_editor_open(s,"Binary inspector / 1024 bytes",source,sizeof source,draft,sizeof draft,0,0))return 3;
    ps_feed(s,"DEADBEEF",8);
    assert(!strcmp(ps_replay_cell(2,0),"╭"));
    assert(!strcmp(ps_replay_cell(5,11),"│"));
    assert(!strcmp(ps_replay_cell(5,82),"█"));
    if(argc==3&&save(argv[1]))return 4;
    ps_key(s,PS_KEY_END);ps_key(s,'A');ps_key(s,'5');assert(draft[1023]==0xA5);
    assert(!strcmp(ps_replay_cell(23,82),"█"));
    assert(ps_hex_editor_set_columns(s,8));ps_key(s,PS_KEY_HOME);
    assert(!strcmp(ps_replay_cell(2,48),"╮"));
    assert(!strcmp(ps_replay_cell(2,80)," "));
    assert(!strcmp(ps_replay_cell(5,82)," "));
    if(argc==3&&save(argv[2]))return 5;
    ps_resize(s,64,28);assert(!strcmp(ps_replay_cell(2,48),"╮"));
    assert(ps_hex_editor_set_columns(s,0));ps_resize(s,96,28);
    assert(!strcmp(ps_replay_cell(2,80),"╮"));
    assert(!ps_hex_editor_set_columns(s,33));
    assert(ps_hex_editor_set_columns(s,32));ps_resize(s,24,8);
    ps_key(s,PS_KEY_END);ps_key(s,'B');ps_key(s,'6');assert(draft[1023]==0xB6);
    /* Plain terminals keep ASCII framing and no reserved track when all rows fit. */
    config.flags=PS_INTERACTIVE;s=ps_init(memory.bytes,sizeof memory.bytes,&config);ps_replay_reset();
    assert(ps_hex_editor_open(s,"Small",source,4,draft,sizeof draft,0,0));
    assert(!strcmp(ps_replay_cell(2,0),"+"));assert(!strcmp(ps_replay_cell(5,82)," "));
    return 0;
}
