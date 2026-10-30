#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "process.h"
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

typedef HRESULT (WINAPI *CreatePty)(COORD,HANDLE,HANDLE,DWORD,HANDLE*);
typedef void (WINAPI *ClosePty)(HANDLE);
typedef HRESULT (WINAPI *ResizePty)(HANDLE,COORD);
typedef struct { HANDLE from,to; } Pump;
static DWORD WINAPI pump(void *arg) {
    Pump *p=(Pump*)arg; char b[2048]; DWORD n,k,at;
    while(ReadFile(p->from,b,sizeof b,&n,0)&&n) {
        for(at=0;at<n;at+=k) if(!WriteFile(p->to,b+at,n-at,&k,0)||!k)return 1;
    }
    return 0;
}
static void clear(void *p,SIZE_T n){unsigned char *b=p;while(n--)*b++=0;}
static BOOL WINAPI control(DWORD kind){return kind==CTRL_C_EVENT||kind==CTRL_BREAK_EVENT;}
/* Windows CRT argv encoding, including empty args, embedded quotes and trailing slashes. */
static int quote(WCHAR *out,unsigned cap,int argc,const char *const *argv) {
    unsigned at=0,i,j,n; static WCHAR word[8192]; int a;
#define PUT(c) do {if(at+1>=cap)return 0;out[at++]=(c);}while(0)
    for(a=0;a<argc;++a){
        if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,argv[a],-1,word,8192))return 0;
        if(a)PUT(' ');
        for(i=0;word[i]&&word[i]!=' '&&word[i]!='\t'&&word[i]!='"';++i){}
        if(i&&!word[i]){for(j=0;j<i;++j)PUT(word[j]);continue;}
        PUT('"');
        for(i=0;word[i];){
            n=0;while(word[i]=='\\'){++n;++i;}
            if(!word[i]||word[i]=='"') {for(j=0;j<n*2;++j)PUT('\\');}
            else {for(j=0;j<n;++j)PUT('\\');}
            if(!word[i])break;
            if(word[i]=='"')PUT('\\');
            PUT(word[i++]);
        }PUT('"');
    }out[at]=0;return 1;
#undef PUT
}
int ps_host_process(int argc,const char *const *argv,int terminal) {
    static WCHAR command[16384]; STARTUPINFOEXW si; PROCESS_INFORMATION pi;
    HANDLE in=GetStdHandle(STD_INPUT_HANDLE),out=GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE childIn=0,parentIn=0,parentOut=0,childOut=0,pty=0,reader=0,writer=0;
    HANDLE inherited[3]={0,0,0}; DWORD im=0,om=0,code=126; SIZE_T bytes=0;
    int ic=GetConsoleMode(in,&im),oc=GetConsoleMode(out,&om),ok=0,i,attributes=0;
    CreatePty create=0;ClosePty close=0;ResizePty resize=0;Pump receive,send;
    COORD size={100,28};CONSOLE_SCREEN_BUFFER_INFO info;
    if(argc<1||!quote(command,16384,argc,argv))return 2;
    clear(&si,sizeof si);clear(&pi,sizeof pi);si.StartupInfo.cb=sizeof si;
    if(terminal){
        /* Explicit NULL std handles let ConPTY supply them even when this
           parent was launched with redirected standard handles. */
        si.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
        HMODULE kernel=GetModuleHandleW(L"kernel32.dll");
        create=(CreatePty)(void*)GetProcAddress(kernel,"CreatePseudoConsole");
        close=(ClosePty)(void*)GetProcAddress(kernel,"ClosePseudoConsole");
        resize=(ResizePty)(void*)GetProcAddress(kernel,"ResizePseudoConsole");
        if(!create||!close||!resize)return 125;
        if(GetConsoleScreenBufferInfo(out,&info)){size.X=info.srWindow.Right-info.srWindow.Left+1;size.Y=info.srWindow.Bottom-info.srWindow.Top+1;}
        if(!CreatePipe(&childIn,&parentIn,0,0)||!CreatePipe(&parentOut,&childOut,0,0))goto done;
        if(FAILED(create(size,childIn,childOut,0,&pty)))goto done;
        InitializeProcThreadAttributeList(0,1,0,&bytes);
        si.lpAttributeList=HeapAlloc(GetProcessHeap(),0,bytes);
        if(!si.lpAttributeList)goto done;
        if(!InitializeProcThreadAttributeList(si.lpAttributeList,1,0,&bytes))goto done;
        attributes=1;
        if(!UpdateProcThreadAttribute(si.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,pty,sizeof pty,0,0))goto done;
        if(ic)SetConsoleMode(in,(im|ENABLE_VIRTUAL_TERMINAL_INPUT)&~(ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT|ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
        if(oc)SetConsoleMode(out,om|ENABLE_VIRTUAL_TERMINAL_PROCESSING|ENABLE_PROCESSED_OUTPUT);
    }else{
        si.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
        for(i=0;i<3;++i){HANDLE h=GetStdHandle(i==0?STD_INPUT_HANDLE:i==1?STD_OUTPUT_HANDLE:STD_ERROR_HANDLE);
            if(!DuplicateHandle(GetCurrentProcess(),h,GetCurrentProcess(),&inherited[i],0,TRUE,DUPLICATE_SAME_ACCESS))goto done;}
        si.StartupInfo.hStdInput=inherited[0];si.StartupInfo.hStdOutput=inherited[1];si.StartupInfo.hStdError=inherited[2];
        if(ic)SetConsoleMode(in,ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_EXTENDED_FLAGS);
    }
    SetConsoleCtrlHandler(control,TRUE);
    ok=CreateProcessW(0,command,0,0,!terminal,terminal?EXTENDED_STARTUPINFO_PRESENT:0,0,0,&si.StartupInfo,&pi);
    if(!ok){code=GetLastError()==ERROR_FILE_NOT_FOUND?127:126;goto done;}
    if(terminal){
        CloseHandle(childIn);childIn=0;CloseHandle(childOut);childOut=0;
        receive.from=parentOut;receive.to=out;send.from=in;send.to=parentIn;
        reader=CreateThread(0,0,pump,&receive,0,0);writer=CreateThread(0,0,pump,&send,0,0);
        if(!reader||!writer){TerminateProcess(pi.hProcess,126);}
        while(WaitForSingleObject(pi.hProcess,50)==WAIT_TIMEOUT){
            if(oc&&GetConsoleScreenBufferInfo(out,&info)){
                COORD next={info.srWindow.Right-info.srWindow.Left+1,info.srWindow.Bottom-info.srWindow.Top+1};
                if(next.X!=size.X||next.Y!=size.Y){resize(pty,next);size=next;}
            }
        }
    }else WaitForSingleObject(pi.hProcess,INFINITE);
    GetExitCodeProcess(pi.hProcess,&code);
done:
    if(writer){do {CancelSynchronousIo(writer);}while(WaitForSingleObject(writer,20)==WAIT_TIMEOUT);CloseHandle(writer);}
    if(parentIn)CloseHandle(parentIn);
    if(childIn){CloseHandle(childIn);childIn=0;}if(childOut){CloseHandle(childOut);childOut=0;}
    if(!reader&&parentOut){CloseHandle(parentOut);parentOut=0;}
    if(pty)close(pty); /* reader remains active while ConPTY drains. */
    if(reader){WaitForSingleObject(reader,INFINITE);CloseHandle(reader);}
    if(parentOut)CloseHandle(parentOut);if(childIn)CloseHandle(childIn);if(childOut)CloseHandle(childOut);
    for(i=0;i<3;++i)if(inherited[i])CloseHandle(inherited[i]);
    if(pi.hThread)CloseHandle(pi.hThread);if(pi.hProcess)CloseHandle(pi.hProcess);
    if(si.lpAttributeList){if(attributes)DeleteProcThreadAttributeList(si.lpAttributeList);HeapFree(GetProcessHeap(),0,si.lpAttributeList);}
    SetConsoleCtrlHandler(control,FALSE);
    if(ic)SetConsoleMode(in,im);if(oc)SetConsoleMode(out,om);
    return (int)code;
}
