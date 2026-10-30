/* Deterministic preview from real editor output; no filesystem in the core. */
#include "portable_shell.h"
#include <stdio.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static union {void *align;ps_byte bytes[PS_DEFAULT_SESSION_STORAGE];} memory;
static char svg[1200000];
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;ps_replay_feed(p,n);}
int main(int argc,char **argv){
    ps_byte source[256],draft[256];unsigned i,n;FILE *f;
    ps_config config={sink,0,96,28,PS_INTERACTIVE|PS_COLOR|PS_UNICODE};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&config);
    if(argc!=2||!s)return 2;
    for(i=0;i<sizeof source;++i)source[i]=(ps_byte)i;
    ps_replay_reset();
    if(!ps_hex_editor_open(s,"Binary inspector / 256 bytes",source,sizeof source,draft,sizeof draft,0,0))return 3;
    ps_feed(s,"DEADBEEF",8);
    n=ps_replay_svg(svg);f=fopen(argv[1],"wb");if(!f)return 4;
    if(fwrite(svg,1,n,f)!=n){fclose(f);return 5;}return fclose(f)!=0;
}
