/* Native demo adapter. All game rules/rendering live in snake_game.h. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "snake_game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>
static volatile sig_atomic_t stopped;
static void stop_signal(int sig){(void)sig;stopped=1;}
#endif
static snake_game game;
static ps_screen screen;
static ps_cell *cells,*presented;
static void output(void *ctx,const unsigned char *bytes,size_t n) {
    (void)ctx;fwrite(bytes,1,n,stdout);
}
static uint32_t milliseconds(void) {
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint32_t)((uint64_t)t.tv_sec*1000u+(unsigned)t.tv_nsec/1000000u);
#endif
}
int main(int argc,char **argv) {
    unsigned unicode=1,bell=1,color=PS_COLOR_RGB,small=0,start_paused=0,wrap=1;int i;
    ps_rich out={output,0,PS_COLOR_RGB,0,0};
#ifdef _WIN32
    HANDLE input=GetStdHandle(STD_INPUT_HANDLE),display=GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD old_input,old_output; UINT old_cp=GetConsoleOutputCP();
#else
    struct termios old_mode,raw;
#endif
    for(i=1;i<argc;++i){
        if(!strcmp(argv[i],"--paused"))start_paused=1;
        else if(!strcmp(argv[i],"--no-wrap"))wrap=0;
        else if(!strcmp(argv[i],"--ascii"))unicode=0;
        else if(!strcmp(argv[i],"--no-bell"))bell=0;
        else if(!strcmp(argv[i],"--no-color"))color=PS_COLOR_NONE;
        else if(!strcmp(argv[i],"--256"))color=PS_COLOR_256;
        else if(!strcmp(argv[i],"--16"))color=PS_COLOR_16;
        else {puts("PortableShellSnake [--ascii] [--no-color|--256|--16] [--no-bell] [--paused] [--no-wrap]\nRequires an interactive terminal of at least 80 columns by 25 rows.");return strcmp(argv[i],"--help")?2:0;}
    }
#ifdef _WIN32
    if(!GetConsoleMode(input,&old_input)||!GetConsoleMode(display,&old_output)){
        fputs("Snake requires an interactive terminal.\n",stderr);return 2;
    }
    if(!SetConsoleMode(display,old_output|ENABLE_VIRTUAL_TERMINAL_PROCESSING))return 2;
    if(!SetConsoleMode(input,ENABLE_WINDOW_INPUT|ENABLE_EXTENDED_FLAGS)){
        SetConsoleMode(display,old_output);return 2;
    }
#else
    if(!isatty(STDIN_FILENO)||!isatty(STDOUT_FILENO)||tcgetattr(STDIN_FILENO,&old_mode)){
        fputs("Snake requires an interactive terminal.\n",stderr);return 2;
    }
    raw=old_mode;raw.c_lflag&=(tcflag_t)~(ICANON|ECHO|ISIG);
    raw.c_iflag&=(tcflag_t)~(IXON|ICRNL);raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=0;
    if(tcsetattr(STDIN_FILENO,TCSANOW,&raw))return 2;
    signal(SIGTERM,stop_signal);signal(SIGINT,stop_signal);signal(SIGHUP,stop_signal);
#endif
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    out.color=color;snake_init(&game,milliseconds(),unicode,bell);game.paused=start_paused;game.wrap=wrap;

    ps_rich_literal(&out,"\033[?1049h\033[?25l\033[2J");
    while(!game.quit){
        uint32_t now=milliseconds();unsigned columns=0,rows=0;
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO info;INPUT_RECORD event;DWORD count;
        if(GetConsoleScreenBufferInfo(display,&info)){
            columns=(unsigned)(info.srWindow.Right-info.srWindow.Left+1);
            rows=(unsigned)(info.srWindow.Bottom-info.srWindow.Top+1);
        }
        if(WaitForSingleObject(input,16)==WAIT_OBJECT_0 && ReadConsoleInputW(input,&event,1,&count) && count){
            if(event.EventType==KEY_EVENT && event.Event.KeyEvent.bKeyDown){
                WORD key=event.Event.KeyEvent.wVirtualKeyCode;
                unsigned mapped=key==VK_UP?SNAKE_UP:key==VK_DOWN?SNAKE_DOWN:key==VK_LEFT?SNAKE_LEFT:key==VK_RIGHT?SNAKE_RIGHT:0;
                if(mapped)snake_key(&game,mapped,now);
                else if(event.Event.KeyEvent.uChar.UnicodeChar<128)
                    snake_feed(&game,(unsigned char)event.Event.KeyEvent.uChar.UnicodeChar,now);
            }
        }
#else
        struct winsize size;fd_set ready;struct timeval timeout={0,16000};
        if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&size)==0){columns=size.ws_col;rows=size.ws_row;}
        FD_ZERO(&ready);FD_SET(STDIN_FILENO,&ready);
        if(select(STDIN_FILENO+1,&ready,0,0,&timeout)>0){
            unsigned char bytes[64];ssize_t n=read(STDIN_FILENO,bytes,sizeof bytes),j;
            if(n==0)break;
            for(j=0;j<n;++j)snake_feed(&game,bytes[j],now);
        }
        if(stopped)break;
#endif
        if(columns<80 || rows<25){
            if(!small){ps_rich_literal(&out,"\033[2J\033[HResize to 80 x 25 to resume. Q to quit.");small=1;}
            game.last=now;fflush(stdout);continue;
        }
        if(small){small=0;ps_rich_literal(&out,"\033[2J");ps_screen_invalidate(&screen);game.last=now;}
        /* Canvas limits are those of portable_screen; leave the final terminal
         * row free so output cannot scroll the alternate-screen viewport. */
        if(columns>512)columns=512;
        if(rows>257)rows=257;
        if(screen.columns!=columns||screen.rows!=rows-1){
            size_t count=(size_t)columns*(rows-1);
            ps_cell *next=(ps_cell*)malloc(count*sizeof *next);
            ps_cell *previous=(ps_cell*)malloc(count*sizeof *previous);
            if(!next||!previous){free(next);free(previous);break;}
            free(cells);free(presented);cells=next;presented=previous;
            ps_screen_init(&screen,cells,presented,columns,rows-1);
            if(!snake_resize(&game,columns,rows-1,now))break;
            ps_rich_literal(&out,"\033[2J\033[H");
        }
        snake_tick(&game,now,&out);snake_render(&game,&screen,&out);fflush(stdout);
        if(ferror(stdout))break;
    }
    ps_rich_literal(&out,"\033[0m\033[?25h\033[?1049l");fflush(stdout);
#ifdef _WIN32
    SetConsoleMode(input,old_input);SetConsoleMode(display,old_output);SetConsoleOutputCP(old_cp);
#else
    tcsetattr(STDIN_FILENO,TCSANOW,&old_mode);
#endif
    free(cells);free(presented);
    return 0;
}
