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
