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
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);
}
static int save(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;
    if(!f)return 1;
    bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,24,0};ps_session *s;unsigned i,col;
    const ps_report_row rows[]={
        {"Records",0,0,PS_REPORT_SECTION},
        {"Columns",0,"5",PS_REPORT_NUMBER},
        {"Size",0,"0.03 MiB",PS_REPORT_NUMBER},
        {"Column",0,"Type",PS_REPORT_SECTION|PS_REPORT_GAP},
        {"id","autoincrement, unsigned","bigint",0},
        {"owner_id","unsigned","bigint",0},
        {"title","required","string",0},
        {"created_at","nullable","datetime",0},
        {"updated_at","nullable","datetime",0},
        {"Index",0,0,PS_REPORT_SECTION|PS_REPORT_GAP},
        {"records_owner","owner_id",0,0},
        {"PRIMARY","id","unique, primary",0},
        {"Foreign key",0,"On update / On delete",PS_REPORT_SECTION|PS_REPORT_GAP},
        {"records_owner","owner_id references id on owners","no action / no action",0}
    };
    assert(ps_sizeof_session()<=sizeof memory.bytes);s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);used=0;
#if PS_REPORT
    assert(ps_print_report(s,rows,14,PS_REPORT_DOTS)==14);
    assert(strstr(output,". . .")&&strstr(output,"owner_id references"));
    assert(!strchr(output,27));
    for(i=0,col=0;i<used;++i){if(output[i]=='\n'){assert(col==95||col==0);col=0;}else if(output[i]!='\r')++col;}
    ps_resize(s,24,24);used=0;ps_print_report(s,rows,14,PS_REPORT_DOTS);
    for(i=0,col=0;i<used;++i){if(output[i]=='\n')col=0;else if(output[i]!='\r'){++col;assert(col<=23);}}
    assert(strstr(output,"records_owner")&&strstr(output,"datetime"));
    used=0;ps_print_report(s,rows,14,PS_REPORT_SPACES);assert(!strstr(output,". ."));
    used=0;assert(ps_execute(s,"status --report")==0&&strstr(output,"Session"));
#else
    (void)i;(void)col;
    assert(ps_print_report(s,rows,14,0)==0&&used==0);
    assert(ps_execute(s,"status --report")!=0);
#endif
    if(argc==1)return 0;
    if(argc!=3)return 2;
    cfg.flags=PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE;s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    used=0;ps_replay_reset();ps_print_report(s,rows,14,PS_REPORT_DOTS);if(save(argv[1]))return 3;
    used=0;ps_replay_reset();ps_execute(s,"status --report");return save(argv[2]);
}
