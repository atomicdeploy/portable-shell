#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE 1
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include "process.h"
static int send_all(int fd,const char *b,int n){while(n){ssize_t k=write(fd,b,(size_t)n);if(k<0&&errno==EINTR)continue;if(k<=0)return 0;b+=k;n-=(int)k;}return 1;}
int ps_host_process(int argc,const char *const *argv,int terminal){
    char *args[65];int i,status=0,master=-1,slave=-1,have_term,changed=0;
    pid_t pid;struct termios saved,cooked;struct winsize size={28,100,0,0};
    struct sigaction ignore,oldint,oldquit;
    if(argc<1||argc>64)return 2;
    for(i=0;i<argc;++i)args[i]=(char*)argv[i];args[argc]=0;
    have_term=tcgetattr(0,&saved)==0;
    if(terminal==PS_PROCESS_AUTO)terminal=have_term&&isatty(1)?PS_PROCESS_PTY:PS_PROCESS_PIPE;
    if(terminal){
        char *name;master=posix_openpt(O_RDWR|O_NOCTTY);
        if(master<0)return 125;
        if(grantpt(master)||unlockpt(master)||(name=ptsname(master))==0||(slave=open(name,O_RDWR|O_NOCTTY))<0){close(master);return 125;}
        (void)ioctl(1,TIOCGWINSZ,&size);(void)ioctl(slave,TIOCSWINSZ,&size);
        if(have_term){cooked=saved;cooked.c_lflag|=ICANON|ECHO|ISIG|IEXTEN;cooked.c_iflag|=ICRNL;cooked.c_oflag|=OPOST|ONLCR;(void)tcsetattr(slave,TCSANOW,&cooked);}
    }else if(have_term){cooked=saved;cooked.c_lflag|=ICANON|ECHO|ISIG|IEXTEN;cooked.c_iflag|=ICRNL;cooked.c_oflag|=OPOST|ONLCR;changed=tcsetattr(0,TCSANOW,&cooked)==0;}
    ignore.sa_handler=SIG_IGN;sigemptyset(&ignore.sa_mask);ignore.sa_flags=0;
    sigaction(SIGINT,&ignore,&oldint);sigaction(SIGQUIT,&ignore,&oldquit);
    pid=fork();
    if(pid==0){
        signal(SIGINT,SIG_DFL);signal(SIGQUIT,SIG_DFL);signal(SIGPIPE,SIG_DFL);signal(SIGTERM,SIG_DFL);
        if(terminal){close(master);if(setsid()<0||ioctl(slave,TIOCSCTTY,0)<0)_exit(126);for(i=0;i<3;++i)if(dup2(slave,i)<0)_exit(126);if(slave>2)close(slave);}
        if(terminal){const char *term=getenv("TERM");if(!term||!*term||ps_text_equal(term,"dumb"))setenv("TERM","xterm-256color",1);}
        execvp(args[0],args);_exit(errno==ENOENT?127:126);
    }
    if(slave>=0)close(slave);
    if(pid<0){status=126<<8;goto done;}
    if(terminal){
        int input_open=1,exited=0;char b[4096];
        if(have_term){struct termios raw=saved;raw.c_lflag&=~(ICANON|ECHO|ISIG|IEXTEN);raw.c_iflag&=~(ICRNL|IXON);raw.c_oflag&=~OPOST;raw.c_cc[VMIN]=1;raw.c_cc[VTIME]=0;changed=tcsetattr(0,TCSANOW,&raw)==0;}
        for(;;){
            struct pollfd p[2]={{master,POLLIN,0},{input_open?0:-1,POLLIN,0}};struct winsize next;
            int ready=poll(p,2,50);
            if(ioctl(1,TIOCGWINSZ,&next)==0&&(next.ws_col!=size.ws_col||next.ws_row!=size.ws_row)){size=next;(void)ioctl(master,TIOCSWINSZ,&size);}
            if(ready<0&&errno!=EINTR)break;
            if(p[0].revents&(POLLIN|POLLHUP|POLLERR)){ssize_t n=read(master,b,sizeof b);if(n<=0)break;if(!send_all(1,b,(int)n))break;}
            if(p[1].revents&(POLLIN|POLLHUP)){ssize_t n=read(0,b,sizeof b);if(n>0){if(!send_all(master,b,(int)n))break;}else{input_open=0;(void)send_all(master,"\004",1);}}
            if(!exited&&waitpid(pid,&status,WNOHANG)==pid)exited=1;
            if(exited&&ready==0)break;
        }
        close(master);master=-1;
        if(!exited)while(waitpid(pid,&status,0)<0&&errno==EINTR){}
    }else while(waitpid(pid,&status,0)<0&&errno==EINTR){}
done:
    if(master>=0)close(master);
    if(changed)tcsetattr(0,TCSANOW,&saved);
    sigaction(SIGINT,&oldint,0);sigaction(SIGQUIT,&oldquit,0);
    return WIFEXITED(status)?WEXITSTATUS(status):WIFSIGNALED(status)?128+WTERMSIG(status):126;
}

int ps_host_pipeline(unsigned count,const ps_shell_command *commands) {
    int pipes[PS_SHELL_COMMANDS][2],files[PS_SHELL_COMMANDS][3],status=0,result=126,failed=1;
    pid_t children[PS_SHELL_COMMANDS];unsigned i,j,started=0;
    struct termios saved,cooked;int terminal=0;
    struct sigaction ignore,oldint,oldquit;int signals=0;
    if(!count||count>PS_SHELL_COMMANDS)return 2;
    for(i=0;i<count;++i){pipes[i][0]=pipes[i][1]=-1;for(j=0;j<3;++j)files[i][j]=-1;}
    for(i=0;i<count;++i){
        const ps_shell_command *c=commands+i;const char *paths[3]={c->input,c->output,c->error};
        for(j=0;j<3;++j)if(paths[j]){
            int append=j==1?c->append:c->append_error;
            files[i][j]=open(paths[j],j==0?O_RDONLY:O_WRONLY|O_CREAT|(append?O_APPEND:O_TRUNC),0666);
            if(files[i][j]<0){result=1;goto cleanup;}
        }
        if(i+1<count&&pipe(pipes[i])<0)goto cleanup;
    }
    terminal=tcgetattr(0,&saved)==0;
    if(terminal){cooked=saved;cooked.c_lflag|=ICANON|ECHO|ISIG|IEXTEN;cooked.c_iflag|=ICRNL;cooked.c_oflag|=OPOST|ONLCR;tcsetattr(0,TCSANOW,&cooked);}
    ignore.sa_handler=SIG_IGN;sigemptyset(&ignore.sa_mask);ignore.sa_flags=0;
    sigaction(SIGINT,&ignore,&oldint);sigaction(SIGQUIT,&ignore,&oldquit);signals=1;
    for(i=0;i<count;++i){
        int report[2];pid_t child;unsigned char launch_error=0;ssize_t got;
        if(pipe(report)<0)goto cleanup;
        if(fcntl(report[1],F_SETFD,FD_CLOEXEC)<0){close(report[0]);close(report[1]);goto cleanup;}
        child=fork();
        if(child<0){close(report[0]);close(report[1]);goto cleanup;}
        if(child==0){
            const ps_shell_command *c=commands+i;
            close(report[0]);
            int in=files[i][0]>=0?files[i][0]:i?pipes[i-1][0]:0;
            int out=files[i][1]>=0?files[i][1]:i+1<count?pipes[i][1]:1;
            int err=c->merge_error?out:files[i][2]>=0?files[i][2]:2;
            signal(SIGINT,SIG_DFL);signal(SIGQUIT,SIG_DFL);signal(SIGPIPE,SIG_DFL);signal(SIGTERM,SIG_DFL);
            if(dup2(in,0)<0||dup2(out,1)<0||dup2(err,2)<0){launch_error=126;(void)write(report[1],&launch_error,1);_exit(126);}
            for(j=0;j<count;++j){unsigned k;
                for(k=0;k<2;++k)if(pipes[j][k]>2)close(pipes[j][k]);
                for(k=0;k<3;++k)if(files[j][k]>2)close(files[j][k]);
            }
            execvp(c->argv[0],(char *const *)c->argv);launch_error=(unsigned char)(errno==ENOENT?127:126);
            (void)write(report[1],&launch_error,1);_exit(launch_error);
        }
        children[started++]=child;close(report[1]);
        do {got=read(report[0],&launch_error,1);}while(got<0&&errno==EINTR);
        close(report[0]);if(got!=0){result=got>0?launch_error:126;goto cleanup;}
    }
    failed=0;
cleanup:
    for(i=0;i<count;++i){for(j=0;j<2;++j)if(pipes[i][j]>=0)close(pipes[i][j]);for(j=0;j<3;++j)if(files[i][j]>=0)close(files[i][j]);}
    if(failed)for(i=0;i<started;++i)kill(children[i],SIGKILL);
    for(i=0;i<started;++i){
        while(waitpid(children[i],&status,0)<0&&errno==EINTR){}
        if(!failed&&i+1==count)result=WIFEXITED(status)?WEXITSTATUS(status):WIFSIGNALED(status)?128+WTERMSIG(status):126;
    }
    if(terminal)tcsetattr(0,TCSANOW,&saved);
    if(signals){sigaction(SIGINT,&oldint,0);sigaction(SIGQUIT,&oldquit,0);}
    return result;
}
