#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static union { void *align; unsigned char bytes[65536]; } storage;
static char output[262144],svg[1000000];static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);}
static void feed(ps_session *s,const char *text){ps_feed(s,text,(unsigned)strlen(text));}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,28,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE};
    ps_session *s=ps_init(storage.bytes,sizeof storage.bytes,&cfg);assert(s);
    ps_replay_reset();ps_set_prompt(s,"command > ",PS_STYLE_PROMPT);
    assert(!ps_execute(s,"board --prompt"));ps_tick(s,100);
    assert(strstr(output,"F4 table")&&strstr(output,"command > "));
    used=0;feed(s,"task add \"Check sensors\"\r");
    assert(strstr(output,"Check sensors"));
    used=0;feed(s,"echo prompt-output\r");assert(strstr(output,"prompt-output"));
    ps_key(s,PS_KEY_UP);ps_key(s,PS_KEY_LEFT);ps_key(s,PS_KEY_RIGHT);
    ps_key(s,3);feed(s,"echo draft");
    ps_key(s,PS_KEY_F4);ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_F4);
    used=0;ps_key(s,13);assert(strstr(output,"draft"));
    used=0;feed(s,"\033[200~echo pasted\033[201~\r");assert(strstr(output,"pasted"));
    ps_resize(s,80,24);ps_tick(s,200);assert(strstr(output,"\033[21;24r"));
    ps_resize(s,96,28);ps_tick(s,300);feed(s,"task ");
    if(argc==2){FILE *f=fopen(argv[1],"wb");unsigned n=ps_replay_svg(svg);assert(f);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));}
    ps_key(s,PS_KEY_F4);used=0;ps_key(s,27);assert(strstr(output,"\033[r"));
    return 0;
}
