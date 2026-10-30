#include "portable_monitor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char output[262144],svg[800000];static unsigned used;
static void sink(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=(unsigned)n;output[used]=0;ps_replay_feed(p,(unsigned)n);
}
static int save(const char *path){unsigned n=ps_replay_svg(svg);FILE*f=fopen(path,"wb");int bad;if(!f)return 1;bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;}
int main(int argc,char **argv){
    ps_log_record records[8];ps_monitor_task tasks[4];ps_monitor m=ps_monitor_init(records,8,tasks,4);
    ps_monitor_style style={96,24,1,1,"Build and deployment | event stream"};
    unsigned i;ps_monitor_append(&m,1,PS_LOG_INFO,"first");
    ps_monitor_action(&m,PS_MONITOR_PAUSE);ps_monitor_append(&m,2,PS_LOG_WARNING,"second");
    assert(m.count==2&&m.offset==1&&m.paused);
    ps_monitor_action(&m,PS_MONITOR_LIVE);assert(!m.offset&&!m.paused);
    for(i=0;i<10;++i)ps_monitor_append(&m,i,PS_LOG_DEBUG,"history");
    assert(m.count==8&&m.dropped==4);
    ps_monitor_action(&m,PS_MONITOR_OLDER);assert(m.offset==1);
    ps_monitor_append(&m,100,PS_LOG_ERROR,"\033[2Junsafe\ntext");
    assert(m.offset==2&&m.dropped==5);
    ps_monitor_action(&m,PS_MONITOR_LIVE);
    used=0;ps_log_write(sink,0,123,PS_LOG_WARNING,"\033[2Jhello\nworld");
    assert(!strchr(output,27)&&strstr(output,"WARN  hello world"));
#if PS_MONITOR_TASKS
    assert(ps_monitor_update(&m,1,PS_TASK_RUNNING,64,100,"Compile modules"));
    assert(!ps_monitor_update(&m,1,PS_TASK_DONE,101,100,"invalid"));
    assert(m.task_count==1&&tasks[0].done==64);
#else
    assert(!ps_monitor_update(&m,1,PS_TASK_RUNNING,64,100,"disabled"));
#endif
    m.minimum=PS_LOG_ERROR;used=0;ps_monitor_render(&m,&style,100,sink,0);
    assert(strstr(output,"unsafe text")&&!strstr(output,"history"));
    style.columns=40;style.rows=12;style.color=style.unicode=0;
    used=0;assert(ps_monitor_render(&m,&style,200,sink,0)==12);
    if(argc==1)return 0;
    if(argc!=3)return 2;
    m=ps_monitor_init(records,8,tasks,4);style.columns=96;style.rows=24;style.color=style.unicode=1;
    ps_monitor_update(&m,1,PS_TASK_DONE,128,128,"Resolve dependencies");
    ps_monitor_update(&m,2,PS_TASK_RUNNING,73,100,"Compile modules");
    ps_monitor_update(&m,3,PS_TASK_RUNNING,0,0,"Wait for target acknowledgment");
    ps_monitor_append(&m,120,PS_LOG_INFO,"Connected to build event producer");
    ps_monitor_append(&m,310,PS_LOG_SUCCESS,"Dependency graph validated");
    ps_monitor_append(&m,615,PS_LOG_WARNING,"Cache unavailable; using source compilation");
    ps_monitor_append(&m,700,PS_LOG_DEBUG,"Worker pool ready: 4 tasks");
    ps_monitor_append(&m,1140,PS_LOG_INFO,"Compilation batch 3 completed");
    ps_monitor_append(&m,1390,PS_LOG_INFO,"Awaiting device readback; no success reported yet");
    ps_replay_reset();used=0;ps_monitor_render(&m,&style,1500,sink,0);if(save(argv[1]))return 3;
    ps_monitor_update(&m,2,PS_TASK_DONE,100,100,"Compile modules");
    ps_monitor_update(&m,3,PS_TASK_FAILED,0,0,"Target acknowledgment timed out");
    ps_monitor_append(&m,3200,PS_LOG_ERROR,"No acknowledgment before deadline; deployment incomplete");
    ps_monitor_append(&m,3210,PS_LOG_INFO,"Previous configuration remains active");
    ps_monitor_render(&m,&style,3210,sink,0);return save(argv[2]);
}
