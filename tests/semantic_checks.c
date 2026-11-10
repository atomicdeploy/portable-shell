#include "portable_shell.h"
#include "portable_style.h"
#include "portable_osc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_foreground(unsigned,unsigned);
unsigned ps_replay_foreground_role(unsigned,unsigned);
void ps_replay_set_accent(unsigned);
static char output[32768];static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;
}
static void bytes(const char *p){while(*p)ps_replay_feed((const unsigned char*)p++,1);}
int main(void){
    ps_config c={sink,0,80,24,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR};
    void *memory=calloc(1,ps_sizeof_session());ps_session *s;unsigned i;
    const char *preserve[]={"\033[48;2;12;34;56m","\033[48;5;38m","\033[1;4m","\033[49m"};
    const char *clear[]={"\033[38;2;0;255;255m","\033[38;5;48m","\033[36m","\033[96m","\033[39m","\033[0m"};
    assert(memory);s=ps_init(memory,ps_sizeof_session(),&c);assert(s);used=0;
    ps_message(s,PS_STYLE_ACCENT,"fallback");assert(!strstr(output,PS_SEMANTIC_ACCENT_SEQUENCE));
    used=0;ps_set_semantic_accent(s,1,0);ps_message(s,PS_STYLE_ACCENT,"accent");
#if PS_SEMANTIC_ACCENT && PS_OSC && !defined(PS_ASCII_ONLY)
    assert(strstr(output,"\033[38;2;216;160;255m" PS_SEMANTIC_ACCENT_SEQUENCE));
#else
    assert(!strstr(output,PS_SEMANTIC_ACCENT_SEQUENCE));
#endif
    used=0;ps_message(s,PS_STYLE_PROMPT,"ordinary cyan");assert(!strstr(output,PS_SEMANTIC_ACCENT_SEQUENCE));
    c.flags|=PS_NO_OSC;s=ps_init(memory,ps_sizeof_session(),&c);used=0;
    ps_set_semantic_accent(s,1,0);ps_message(s,PS_STYLE_ACCENT,"disabled");assert(!strstr(output,PS_SEMANTIC_ACCENT_SEQUENCE));
    c.flags=0;s=ps_init(memory,ps_sizeof_session(),&c);used=0;
    ps_set_semantic_accent(s,1,0);ps_message(s,PS_STYLE_ACCENT,"batch");assert(!strchr(output,27));
    /* One byte at a time, including split OSC introducer and ST terminator. */
    ps_replay_reset();bytes("\033[38;2;216;160;255m" PS_SEMANTIC_ACCENT_SEQUENCE "A\033[36mC");
    assert(ps_replay_foreground_role(0,0)==PS_SEMANTIC_ACCENT_FOREGROUND);
    assert(!ps_replay_foreground_role(0,1));
    {unsigned cyan=ps_replay_foreground(0,1);ps_replay_set_accent(0xff3366);
        assert(ps_replay_foreground(0,0)==0xff3366&&ps_replay_foreground(0,1)==cyan);}
    for(i=0;i<PS_COUNT_OF(preserve);++i){ps_replay_reset();bytes(PS_SEMANTIC_ACCENT_SEQUENCE);bytes(preserve[i]);bytes("A");assert(ps_replay_foreground_role(0,0)==PS_SEMANTIC_ACCENT_FOREGROUND);}
    for(i=0;i<PS_COUNT_OF(clear);++i){ps_replay_reset();bytes(PS_SEMANTIC_ACCENT_SEQUENCE);bytes(clear[i]);bytes("A");assert(!ps_replay_foreground_role(0,0));}
    ps_replay_reset();bytes("\033]777;ps;ink;accent-extra\033\\X");assert(!ps_replay_foreground_role(0,0));
    ps_replay_reset();bytes(PS_SEMANTIC_ACCENT_SEQUENCE "\033[2K");assert(ps_replay_foreground_role(0,3)==PS_SEMANTIC_ACCENT_FOREGROUND);
    free(memory);return 0;
}
