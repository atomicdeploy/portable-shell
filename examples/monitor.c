/* Optional hosted C11 adapter. No OS-specific API; producers supply newline events.
 * Default is append-only stdout. --live explicitly enables ANSI alternate screen. */
#include "portable_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static void output(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;
#ifdef _WIN32
    /* stdout text mode already expands LF into CRLF. */
    while(n--){if(*p!='\r'||!n||p[1]!='\n')fputc(*p,stdout);++p;}
#else
    fwrite(p,1,n,stdout);
#endif
}
static uint32_t elapsed(struct timespec start){
    struct timespec now;timespec_get(&now,TIME_UTC);
    return (uint32_t)((now.tv_sec-start.tv_sec)*1000+(now.tv_nsec-start.tv_nsec)/1000000);
}
int main(int argc,char **argv){
    ps_log_record records[64];ps_monitor_task tasks[8];
    ps_monitor m=ps_monitor_init(records,64,tasks,8);
    ps_monitor_style style={96,24,1,1,"Event monitor"};
    struct timespec start;char line[512];unsigned live=0;int i;
    timespec_get(&start,TIME_UTC);
    for(i=1;i<argc;++i){
        if(!strcmp(argv[i],"--live"))live=1;
        else if(!strcmp(argv[i],"--ascii"))style.unicode=0;
        else if(!strcmp(argv[i],"--no-color"))style.color=0;
        else if(!strcmp(argv[i],"--help")){
            puts("PortableShellMonitor [--live] [--ascii] [--no-color]\n"
                 "stdin: INFO|WARN|ERROR|OK|DEBUG message\n"
                 "       TASK id done total WAIT|RUN|DONE|FAIL label\n"
                 "       :older :newer :pause :live :level :quit");return 0;
        }else return 2;
    }
    if(live)ps_monitor_open(output,0);
    while(fgets(line,sizeof line,stdin)){
        unsigned level=PS_LOG_INFO,id,done,total,state;char state_name[8],label[160],*text=line;uint32_t time=elapsed(start);
        size_t n=strlen(line);
        if(n==sizeof line-1 && line[n-1]!='\n'){
            int tail;while((tail=fgetc(stdin))!=EOF&&tail!='\n'){}
            line[n-3]='.';line[n-2]='.';line[n-1]='.';
        }
        while(n&&(line[n-1]=='\r'||line[n-1]=='\n'))line[--n]=0;
        if(!strcmp(line,":quit"))break;
        if(!strcmp(line,":older"))ps_monitor_action(&m,PS_MONITOR_OLDER);
        else if(!strcmp(line,":newer"))ps_monitor_action(&m,PS_MONITOR_NEWER);
        else if(!strcmp(line,":pause"))ps_monitor_action(&m,PS_MONITOR_PAUSE);
        else if(!strcmp(line,":live"))ps_monitor_action(&m,PS_MONITOR_LIVE);
        else if(!strcmp(line,":level"))ps_monitor_action(&m,PS_MONITOR_FILTER);
        else if(sscanf(line,"TASK %u %u %u %7s %159[^\n]",&id,&done,&total,state_name,label)==5){
            state=!strcmp(state_name,"RUN")?PS_TASK_RUNNING:!strcmp(state_name,"DONE")?PS_TASK_DONE:
                !strcmp(state_name,"WAIT")?PS_TASK_WAITING:!strcmp(state_name,"FAIL")?PS_TASK_FAILED:99;
            if(!ps_monitor_update(&m,id,state,done,total,label)){
                ps_monitor_append(&m,time,PS_LOG_ERROR,"Task update rejected (range, state or capacity)");
                if(!live)ps_log_write(output,0,time,PS_LOG_ERROR,"Task update rejected (range, state or capacity)");
            }else if(!live)ps_log_write(output,0,time,PS_LOG_INFO,line);
        }else{
            if(!strncmp(text,"WARN ",5)){level=PS_LOG_WARNING;text+=5;}
            else if(!strncmp(text,"ERROR ",6)){level=PS_LOG_ERROR;text+=6;}
            else if(!strncmp(text,"OK ",3)){level=PS_LOG_SUCCESS;text+=3;}
            else if(!strncmp(text,"DEBUG ",6)){level=PS_LOG_DEBUG;text+=6;}
            else if(!strncmp(text,"INFO ",5))text+=5;
            ps_monitor_append(&m,time,level,text);
            if(!live)ps_log_write(output,0,time,level,text);
        }
        if(live)ps_monitor_render(&m,&style,time,output,0);
        fflush(stdout);
    }
    if(live)ps_monitor_close(output,0);
    return ferror(stdin)||ferror(stdout)?1:0;
}
