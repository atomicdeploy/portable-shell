#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char output[32768],svg[800000];static unsigned used;
static union {void *alignment;unsigned char bytes[50000];} memory;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;
    ps_replay_feed(p,n);
}
static int save(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;
    if(!f)return 1;
    bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
}
static int run(ps_session *s,int argc,const char *const *argv,void *ctx){
    (void)s;(void)argc;(void)argv;(void)ctx;return 0;
}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,36,0};ps_session *s;
    ps_help_item options[]={{"--long-option VALUE","A long description that must wrap beneath its own description column without touching the option label."}};
    ps_help_document docs[]={{"sample","A custom command.",0,0,options,1,0,0}};
    ps_command cmd={"sample","[OPTIONS]","Custom",run,0,0};
    assert(ps_sizeof_session()<=sizeof memory.bytes);
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(s);ps_register(s,&cmd);ps_set_help_documents(s,docs,1);
    used=0;assert(ps_execute(s,"help sample")==0);
#if PS_HELP_DETAILS
    assert(strstr(output,"A custom command.")&&strstr(output,"Options:"));
#endif
    assert(!strchr(output,27));
    ps_resize(s,40,36);used=0;ps_execute(s,"help sample");
    {unsigned i,col=0;for(i=0;i<used;++i){if(output[i]=='\n')col=0;else if(output[i]!='\r'){++col;assert(col<40);}}}
    ps_set_help_documents(s,0,0);used=0;ps_execute(s,"help sample");assert(!strstr(output,"A custom command."));
    if(argc==1)return 0;
    if(argc!=4)return 2;
    cfg.flags=PS_COLOR|PS_TRUECOLOR|PS_INTERACTIVE;cfg.columns=96;
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    used=0;ps_replay_reset();ps_execute(s,"help");if(save(argv[1]))return 3;
    used=0;ps_replay_reset();ps_execute(s,"help segments");if(save(argv[2]))return 4;
    cfg.flags=PS_INTERACTIVE;cfg.columns=64;s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    used=0;ps_replay_reset();ps_execute(s,"help segments");return save(argv[3]);
}
