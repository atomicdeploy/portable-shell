/* Hosted subprocess fixture: checks real OS streams, argv and terminal allocation. */
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
int main(int argc,char **argv){
    int i,tty;char line[256];
#ifdef _WIN32
    DWORD mode;tty=GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&mode)!=0;
#else
    tty=isatty(0);
#endif
    if(argc>1&&!strcmp(argv[1],"input")){
        if(!fgets(line,sizeof line,stdin))return 8;
        printf("input:%s",line);return 0;
    }
    printf("tty:%d\n",tty);
    for(i=1;i<argc;++i)printf("arg:%u:%s\n",(unsigned)strlen(argv[i]),argv[i]);
    fprintf(stderr,"stderr-marker\n");
    return argc>1&&!strcmp(argv[1],"fail")?7:0;
}
