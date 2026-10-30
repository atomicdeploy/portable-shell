#include "portable_shell.h"
#include "portable_osc.h"
#include <assert.h>
#include <string.h>
static char output[32768];static unsigned used;
static union {void *align;unsigned char bytes[65536];} storage;
static void sink(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=(unsigned)n;output[used]=0;
}
static void shell_sink(void *ctx,const ps_byte *p,unsigned n){sink(ctx,p,n);}
static void clear(void){used=0;output[0]=0;}
int main(void){
    ps_config config={0};ps_session *s;
    if(!PS_OSC){assert(!ps_terminal_title(sink,0,2,"disabled"));assert(!used);return 0;}
    assert(ps_terminal_title(sink,0,2,"Test \xe2\x9c\x93"));
    assert(!strcmp(output,"\033]2;Test \xe2\x9c\x93\033\\"));clear();
    assert(!ps_terminal_title(sink,0,2,"bad\033]52;c;secret\a"));
    assert(!ps_terminal_title(sink,0,2,"bad\xc2\x9c"));
    assert(!ps_terminal_title(sink,0,2,"bad\xf0\x80"));
    assert(!ps_terminal_title(sink,0,3,"title"));assert(!used);
    assert(ps_terminal_directory(sink,0,"host","/a b/#%\xc3\xa9"));
    assert(!strcmp(output,"\033]7;file://host/a%20b/%23%25%C3%A9\033\\"));clear();
    assert(!ps_terminal_directory(sink,0,"host;bad","/tmp"));
    assert(!ps_terminal_directory(sink,0,"host","relative"));assert(!used);
    assert(ps_terminal_progress(sink,0,4,81));
    assert(!strcmp(output,"\033]9;4;4;81\033\\"));clear();
    assert(!ps_terminal_progress(sink,0,5,0));assert(!ps_terminal_progress(sink,0,1,101));assert(!used);
    config.write=shell_sink;config.columns=80;config.rows=24;config.flags=PS_INTERACTIVE;
    s=ps_init(storage.bytes,sizeof storage.bytes,&config);assert(s);clear();
    assert(!ps_execute(s,"title Hello terminal"));
    assert(!strcmp(output,"\033]2;Hello terminal\033\\"));clear();
    assert(!ps_execute(s,"osc progress normal 42"));
    assert(!strcmp(output,"\033]9;4;1;42\033\\"));clear();
    ps_progress(s,"Transfer",1,4);assert(!strcmp(output,"\033]9;4;1;25\033\\"));clear();
    ps_progress(s,"Transfer",4,4);assert(!strcmp(output,"\033]9;4;0;100\033\\"));clear();
    config.flags=PS_INTERACTIVE|PS_NO_OSC;s=ps_init(storage.bytes,sizeof storage.bytes,&config);clear();
    assert(!ps_set_title(s,"hidden"));assert(!ps_terminal_report_progress(s,1,5));assert(!used);
    config.flags=0;s=ps_init(storage.bytes,sizeof storage.bytes,&config);clear();
    assert(!ps_execute(s,"title batch"));assert(!used);
    return 0;
}

