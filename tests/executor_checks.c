#include "portable_shell.h"
#include <assert.h>
#include <string.h>
static union {void *align;unsigned char bytes[PS_DEFAULT_SESSION_STORAGE];} storage;
static char output[8192];static unsigned used,calls;
static void write_bytes(void *ctx,const ps_byte *b,unsigned n){(void)ctx;assert(used+n<sizeof output);memcpy(output+used,b,n);used+=n;output[used]=0;}
static int execute(ps_session *s,int argc,const char *const *argv,void *ctx){(void)s;assert(ctx==&calls);assert(argc==2);assert(!strcmp(argv[0],"external"));assert(!strcmp(argv[1],"a b"));++calls;return 9;}
static void feed(ps_session *s,const char *p){ps_feed(s,(const ps_byte*)p,(unsigned)strlen(p));}
int main(void){
    ps_config cfg={write_bytes,0,80,24,0};ps_session *s=ps_init(storage.bytes,sizeof storage.bytes,&cfg);
    assert(s);ps_set_executor(s,execute,&calls);assert(ps_execute(s,"external 'a b'")==9);assert(calls==1);
    assert(ps_execute(s,"echo builtin")==0&&calls==1);ps_set_executor(s,0,0);assert(ps_execute(s,"external")==2);
    cfg.flags=PS_INTERACTIVE|PS_EXTENDED_KEYS;s=ps_init(storage.bytes,sizeof storage.bytes,&cfg);ps_start(s);
    feed(s,"echo ab\024\r");assert(!strcmp(ps_history_get(s,0),"echo ba"));
    feed(s,"echo one two\033b\033d\031\r");assert(!strcmp(ps_history_get(s,1),"echo one two"));
    feed(s,"echo one two\033\177\r");assert(!strcmp(ps_history_get(s,2),"echo one "));
    feed(s,"echo a\xc3\xa9\024\r");assert(!strcmp(ps_history_get(s,3),"echo \xc3\xa9" "a"));
    return 0;
}
