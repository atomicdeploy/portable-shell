/* Included by the local adapter after its output queue implementation. */
#include "process.h"
static int host_execute(ps_session *s,int argc,const char *const *argv,void *context){
    int status; (void)context;
    flush(&channels[0]);
    status=ps_host_process(argc,argv,0);
    if(status==127||status==126||status==125){ps_write(s,"Process unavailable (exit ");ps_number(s,(unsigned)status);ps_write(s,")");ps_newline(s);}
    return status;
}
static int host_command(ps_session *s,int argc,const char *const *argv,void *context){
    int status;(void)context;
    if(argc<2)return 2;
    flush(&channels[0]);
    status=ps_host_process(argc-1,argv+1,ps_text_equal(argv[0],"pty"));
    if(status==125){ps_write(s,"PTY unavailable on this host");ps_newline(s);}
    return status;
}
static int host_remote(ps_session *s,int argc,const char *const *argv,void *context){
    const char *args[20];int n=0;(void)context;
    if(argc<4)return 2;
    if(ps_text_equal(argv[1],"ssh")){
        if(argc!=4||argv[2][0]=='-')return 2;
        args[n++]="ssh";args[n++]="--";args[n++]=argv[2];args[n++]=argv[3];
    }else if(ps_text_equal(argv[1],"get")||ps_text_equal(argv[1],"put")||ps_text_equal(argv[1],"post")){
        if(argc!=4)return 2;
        args[n++]="curl";args[n++]="--fail";args[n++]="--show-error";args[n++]="--silent";
        args[n++]="--connect-timeout";args[n++]="10";args[n++]="--max-time";args[n++]="60";
        args[n++]="--proto";args[n++]="=http,https,sftp";
        if(ps_text_equal(argv[1],"get")){args[n++]="--output";args[n++]=argv[3];}
        else if(ps_text_equal(argv[1],"put")){args[n++]="--upload-file";args[n++]=argv[3];}
        else {args[n++]="--header";args[n++]="Content-Type: text/plain";args[n++]="--data-binary";args[n++]=argv[3];}
        args[n++]="--url";args[n++]=argv[2];
    }else return 2;
    return host_execute(s,n,args,0);
}
#if PS_HOST_LANGUAGE
#include "language.h"
#endif
static int host_attach(ps_session *s){
    ps_command run={"run","PROGRAM [ARGS...]","Host",host_command,0,0};
    ps_command pty={"pty","PROGRAM [ARGS...]","Host",host_command,0,0};
    ps_command remote={"remote","get|put URL FILE | post URL BODY | ssh HOST COMMAND","Host",host_remote,0,0};
    if(ps_register(s,&run)||ps_register(s,&pty)||ps_register(s,&remote))return 1;
    #if PS_HOST_LANGUAGE
    {
        const char *names[]={"cd","pwd","env","setenv","unsetenv","source","exit"};
        const char *usage[]={"[PATH|-]","","NAME","NAME VALUE","NAME","FILE","[STATUS]"};
        unsigned i;for(i=0;i<7;++i){
            ps_command c={names[i],usage[i],"Host",host_state_command,0,0};
            if(ps_register(s,&c))return 1;
        }
    }
    ps_set_line_executor(s,host_line,0);
    host_report_directory(s);
    #endif
    ps_set_executor(s,host_execute,0);return 0;
}
