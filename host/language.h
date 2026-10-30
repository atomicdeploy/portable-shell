/* Local-host command language. Never installed on serial/Telnet sessions. */
#include "portable_command_line.h"
#ifndef _WIN32
#include <stdlib.h>
#endif
static char host_self_path[4096];
static void host_program(const char *name) {
#ifdef _WIN32
    static WCHAR wide[4096];DWORD n=GetModuleFileNameW(0,wide,4096);(void)name;
    if(n&&n<4096)WideCharToMultiByte(CP_UTF8,0,wide,-1,host_self_path,sizeof host_self_path,0,0);
#else
    unsigned slash=0;while(name[slash]&&name[slash]!='/')++slash;
    if(!name[slash]||!realpath(name,host_self_path)){
        const char *path=getenv("PATH");char candidate[4096];unsigned n;
        host_self_path[0]=0;
        while(path&&*path){
            n=0;while(*path&&*path!=':'){if(n+1<sizeof candidate)candidate[n++]=*path;++path;}
            if(*path==':')++path;
            if(n&&n+1<sizeof candidate)candidate[n++]='/';
            if(n+ps_text_length(name)+1>=sizeof candidate)continue;
            ps_bytes_copy(candidate+n,name,ps_text_length(name)+1);
            if(access(candidate,X_OK)==0&&realpath(candidate,host_self_path))break;
        }
    }
#endif
}
static unsigned host_script_depth;
static char host_previous_directory[4096];
static const char *host_env_lookup(void *context,const char *name) {
    int status=context?*(const int *)context:0;static char value[8192],number[16];
    if(ps_text_equal(name,"?")){
        unsigned n=(unsigned)status,i=0,j;
        do{number[i++]=(char)('0'+n%10);n/=10;}while(n);
        for(j=0;j<i/2;++j){char c=number[j];number[j]=number[i-j-1];number[i-j-1]=c;}number[i]=0;return number;
    }
#ifdef _WIN32
    {static WCHAR key[128],wide[4096];DWORD n;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name,-1,key,128))return "";
    n=GetEnvironmentVariableW(key,wide,4096);
    if(!n||n>=4096||!WideCharToMultiByte(CP_UTF8,0,wide,-1,value,sizeof value,0,0))return "";
    return value;}
#else
    (void)value;{const char *v=getenv(name);return v?v:"";}
#endif
}
static int host_directory(char *path,unsigned capacity) {
#ifdef _WIN32
    static WCHAR wide[4096];DWORD n=GetCurrentDirectoryW(4096,wide);
    return n&&n<4096&&WideCharToMultiByte(CP_UTF8,0,wide,-1,path,(int)capacity,0,0);
#else
    return getcwd(path,capacity)!=0;
#endif
}
static int host_change_directory(const char *path) {
#ifdef _WIN32
    static WCHAR wide[4096];
    return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,4096)&&SetCurrentDirectoryW(wide);
#else
    return chdir(path)==0;
#endif
}
typedef struct {
#ifdef _WIN32
    HANDLE file;
#else
    int file;
#endif
    unsigned failed;
} HostFile;
static int host_file_open(HostFile *f,const char *path,unsigned reading,unsigned append) {
    f->failed=0;
#ifdef _WIN32
    {static WCHAR wide[4096];
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,4096))return 0;
    f->file=CreateFileW(wide,reading?GENERIC_READ:append?FILE_APPEND_DATA:GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,
                       reading?OPEN_EXISTING:append?OPEN_ALWAYS:CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(f->file==INVALID_HANDLE_VALUE)return 0;
    return 1;}
#else
    f->file=open(path,reading?O_RDONLY:O_WRONLY|O_CREAT|(append?O_APPEND:O_TRUNC),0666);
    return f->file>=0;
#endif
}
static void host_file_close(HostFile *f) {
#ifdef _WIN32
    CloseHandle(f->file);
#else
    close(f->file);
#endif
}
static void host_file_write(void *context,const ps_byte *bytes,unsigned n) {
    HostFile *f=(HostFile *)context;
    while(n){
#ifdef _WIN32
        DWORD k;if(!WriteFile(f->file,bytes,n,&k,0)||!k){f->failed=1;return;}
#else
        ssize_t k=write(f->file,bytes,n);if(k<0&&errno==EINTR)continue;
        if(k<=0){f->failed=1;return;}
#endif
        bytes+=k;n-=(unsigned)k;
    }
}
static void host_report_directory(ps_session *s){
    static char path[4098];unsigned i;
    if(!host_directory(path+1,sizeof path-1))return;
#ifdef _WIN32
    path[0]='/';
    for(i=1;path[i];++i)if(path[i]=='\\')path[i]='/';
    /* UNC paths carry the server as the authority. */
    if(path[1]=='/'&&path[2]=='/'){
        char host[256];unsigned n=0;
        for(i=3;path[i]&&path[i]!='/';++i){if(n+1>=sizeof host)return;host[n++]=path[i];}
        host[n]=0;if(path[i])ps_report_directory(s,host,path+i);
    }else ps_report_directory(s,"",path);
#else
    (void)i;ps_report_directory(s,"",path+1);
#endif
}
static int host_state_command(ps_session *s,int argc,const char *const *argv,void *context) {
    const char *name=argv[0];int current=ps_status(s);(void)context;
    if(ps_text_equal(name,"exit")){
        unsigned i,value=0;
        if(argc>2)return 2;
        if(argc==2){
            if(!argv[1][0])return 2;
            for(i=0;argv[1][i];++i){if(argv[1][i]<'0'||argv[1][i]>'9')return 2;value=value*10u+(unsigned)(argv[1][i]-'0');if(value>255)return 2;}
        }else value=(unsigned)current;
        ps_request_exit(s);return (int)value;
    }
    if(ps_text_equal(name,"pwd")){
        static char path[4096];if(argc!=1)return 2;
        if(!host_directory(path,sizeof path))return 1;ps_write(s,path);ps_newline(s);return 0;
    }
    if(ps_text_equal(name,"cd")){
        static char previous[4096];const char *path;
        if(argc>2)return 2;
        path=argc==2?argv[1]:host_env_lookup(&current,
#ifdef _WIN32
            "USERPROFILE"
#else
            "HOME"
#endif
        );
        if(ps_text_equal(path,"-"))path=host_previous_directory;
        if(!*path||!host_directory(previous,sizeof previous)||!host_change_directory(path)){
            ps_write(s,"Directory unavailable");ps_newline(s);return 1;
        }
        ps_bytes_copy(host_previous_directory,previous,ps_text_length(previous)+1);host_report_directory(s);return 0;
    }
    if(ps_text_equal(name,"env")){
        if(argc!=2)return 2;ps_write(s,host_env_lookup(&current,argv[1]));ps_newline(s);return 0;
    }
    if(ps_text_equal(name,"setenv")||ps_text_equal(name,"unsetenv")){
        unsigned i,remove=ps_text_equal(name,"unsetenv");
        if(argc!=(remove?2:3)||!argv[1][0]||(argv[1][0]>='0'&&argv[1][0]<='9'))return 2;
        for(i=0;argv[1][i];++i)if(!ps_shell_name((unsigned char)argv[1][i]))return 2;
#ifdef _WIN32
        {static WCHAR key[128],value[4096];
        if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,argv[1],-1,key,128))return 2;
        if(!remove&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,argv[2],-1,value,4096))return 2;
        return SetEnvironmentVariableW(key,remove?0:value)?0:1;}
#else
        return (remove?unsetenv(argv[1]):setenv(argv[1],argv[2],1))?1:0;
#endif
    }
    if(ps_text_equal(name,"source")){
        HostFile file;char line[PS_LINE];unsigned used=0,overflow=0;int status=0;
        if(argc!=2||host_script_depth>=8)return 2;
        if(!host_file_open(&file,argv[1],1,0)){ps_write(s,"Cannot open script");ps_newline(s);return 1;}
        ++host_script_depth;
        while(!ps_exited(s)){
            unsigned char byte;unsigned got;
#ifdef _WIN32
            DWORD n;if(!ReadFile(file.file,&byte,1,&n,0)){status=1;break;}got=(unsigned)n;
#else
            ssize_t n=read(file.file,&byte,1);if(n<0&&errno==EINTR)continue;
            if(n<0){status=1;break;}got=(unsigned)n;
#endif
            if(got&&!byte){status=2;break;}
            if(!got||byte=='\n'){
                if(overflow){status=2;ps_write(s,"Script line too long");ps_newline(s);break;}
                if(used){line[used]=0;status=ps_execute(s,line);used=0;}
                if(!got)break;
            }else if(byte!='\r'){
                if(used+1<sizeof line)line[used++]=(char)byte;else overflow=1;
            }
        }
        --host_script_depth;host_file_close(&file);return status;
    }
    return 2;
}
static int host_is_command(ps_session *s,const char *name) {
    unsigned i;for(i=0;i<ps_command_count(s);++i)
        if(ps_text_equal(ps_command_at(s,i)->name,name))return 1;
    return 0;
}
static int host_stream_command(const char *name) {
    return ps_text_equal(name,"echo")||ps_text_equal(name,"pwd")||ps_text_equal(name,"env")||
           ps_text_equal(name,"ls")||ps_text_equal(name,"help")||ps_text_equal(name,"status")||
           ps_text_equal(name,"calc")||ps_text_equal(name,"history");
}
typedef struct {ps_shell_plan plan;char values[PS_SHELL_COMMANDS][4096];} HostPlan;
static int host_line_run(ps_session *s,const char *line,HostPlan *work) {
    ps_shell_plan *plan=&work->plan;char (*values)[4096]=work->values;unsigned first=0,end,condition=PS_CHAIN_ALWAYS;int result=ps_status(s);

    /* Validate syntax for the entire record before any command or file side effect. */
    if(!ps_shell_parse(line,plan,0,0))goto syntax;
    while(first<plan->count&&!ps_exited(s)){
        unsigned i;
        /* Re-expand after each executed group: setenv X ...; echo "$X" works. */
        if(!ps_shell_parse(line,plan,host_env_lookup,&result))goto syntax;
        end=first;while(plan->commands[end].next==PS_CHAIN_PIPE)++end;
        if(condition==PS_CHAIN_ALWAYS||(condition==PS_CHAIN_AND&&result==0)||(condition==PS_CHAIN_OR&&result!=0)){
            ps_shell_command *c=plan->commands+first;
            unsigned native=end>first||c->input||c->output||c->error||c->merge_error;
            for(i=first;i<=end;++i){
                ps_shell_command *stage=plan->commands+i;
                if(ps_text_equal(stage->argv[0],"run")){
                    int k;if(stage->argc<2)return 2;
                    for(k=0;k<stage->argc;++k)stage->argv[k]=stage->argv[k+1];--stage->argc;native=1;
                }else if(native&&host_is_command(s,stage->argv[0])){
                    if(first!=end||stage->input||stage->error||stage->merge_error){
                        int k;
                        if(!host_self_path[0]||!host_stream_command(stage->argv[0])||
                           ps_text_equal(stage->argv[0],"history")||ps_text_equal(stage->argv[0],"status")){
                            ps_write(s,"This built-in cannot run as a pipeline stage");ps_newline(s);return 2;
                        }
                        if(ps_text_equal(stage->argv[0],"pwd")){
                            if(stage->argc!=1||!host_directory(values[i],sizeof values[i]))return 2;
                            stage->argc=2;stage->argv[0]="echo";stage->argv[1]=values[i];
                        }else if(ps_text_equal(stage->argv[0],"env")){
                            const char *v;if(stage->argc!=2)return 2;v=host_env_lookup(&result,stage->argv[1]);
                            if(ps_text_length(v)>=sizeof values[i])return 2;
                            ps_bytes_copy(values[i],v,ps_text_length(v)+1);
                            stage->argv[0]="echo";stage->argv[1]=values[i];
                        }
                        for(k=stage->argc;k>=0;--k)stage->argv[k+3]=stage->argv[k];
                        stage->argv[0]=host_self_path;stage->argv[1]="--no-exec";stage->argv[2]="--";stage->argc+=3;
                    }else{
                        if(!host_stream_command(stage->argv[0])||stage->input||stage->error||stage->merge_error){
                            ps_write(s,"This built-in does not support this redirection");ps_newline(s);return 2;
                        }
                        native=0;
                    }
                }
            }
            flush(&channels[0]);
            if(native && end==first && !c->input && !c->output && !c->error && !c->merge_error)
                result=ps_host_process(c->argc,c->argv,PS_PROCESS_AUTO);
            else if(native)result=ps_host_pipeline(end-first+1,c);
            else if(c->output){
                HostFile file;ps_write_fn writer=host_file_write;void *ctx=&file;unsigned old_flags;
                if(!host_file_open(&file,c->output,0,c->append)){ps_write(s,"Cannot open output");ps_newline(s);result=1;}
                else {old_flags=ps_output_flags(s,0);ps_exchange_output(s,&writer,&ctx);result=ps_execute_argv(s,c->argc,c->argv);
                      ps_exchange_output(s,&writer,&ctx);ps_output_flags(s,old_flags);host_file_close(&file);if(file.failed)result=1;}
            }else result=ps_execute_argv(s,c->argc,c->argv);
            ps_set_status(s,result);
        }
        condition=plan->commands[end].next;first=end+1;
    }
    return result;
syntax:
    ps_write(s,plan->error);ps_newline(s);return 2;
}

static int host_line(ps_session *s,const char *line,void *context) {
    HostPlan *work;int result;(void)context;
#ifdef _WIN32
    work=(HostPlan *)HeapAlloc(GetProcessHeap(),0,sizeof *work);
#else
    work=(HostPlan *)malloc(sizeof *work);
#endif
    if(!work){ps_write(s,"Host command storage unavailable");ps_newline(s);return 1;}
    result=host_line_run(s,line,work);
#ifdef _WIN32
    HeapFree(GetProcessHeap(),0,work);
#else
    free(work);
#endif
    return result;
}
