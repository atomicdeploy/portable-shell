#ifndef PS_HOST_FILESYSTEM_H
#define PS_HOST_FILESYSTEM_H
/* Optional native adapter. Never included by the freestanding core.
 * Read-only filesystem enumeration; no deletion, writes or implicit file execution. */
#include "portable_shell.h"
#include "portable_terminal.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif
#define FS_PATH 1024
#define FS_NAME 512
static int fs_copy(char *dst,unsigned cap,const char *src){unsigned i=0;while(src[i]){if(i+1>=cap)return 0;dst[i]=src[i];++i;}dst[i]=0;return 1;}
static int fs_join(char *dst,const char *path,const char *name){unsigned n=(unsigned)ps_text_length(path);if(n+ps_text_length(name)+2>FS_PATH)return 0;fs_copy(dst,FS_PATH,path);if(n&&dst[n-1]!='/'&&dst[n-1]!='\\')dst[n++]='/';return fs_copy(dst+n,FS_PATH-n,name);}
typedef int (*fs_emit_fn)(void *,const char *,unsigned);
/* 1 complete, 2 deliberately stopped, 0 error. Symlinks/reparse points never traversed. */
static int fs_scan(const char *path,fs_emit_fn emit,void *ctx){
#ifdef _WIN32
    WCHAR wide[FS_PATH],pattern[FS_PATH+3];WIN32_FIND_DATAW data;HANDLE h;unsigned i;int result=1;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,FS_PATH))return 0;
    {DWORD attrs=GetFileAttributesW(wide);if(attrs==INVALID_FILE_ATTRIBUTES||!(attrs&FILE_ATTRIBUTE_DIRECTORY)||(attrs&FILE_ATTRIBUTE_REPARSE_POINT))return 0;}
    for(i=0;wide[i];++i)pattern[i]=wide[i];
    pattern[i++]='\\';pattern[i++]='*';pattern[i]=0;
    h=FindFirstFileW(pattern,&data);if(h==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND?1:0;
    do{char name[FS_NAME];unsigned flags=0;
        if(data.cFileName[0]=='.'&&(!data.cFileName[1]||(data.cFileName[1]=='.'&&!data.cFileName[2])))continue;
        if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,data.cFileName,-1,name,sizeof name,0,0)){result=0;break;}
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)flags|=PS_FILE_DIRECTORY;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)flags|=PS_FILE_LINK|PS_FILE_DISABLED;
        if(!emit(ctx,name,flags)){result=2;break;}
    }while(FindNextFileW(h,&data));
    if(result==1&&GetLastError()!=ERROR_NO_MORE_FILES)result=0;
    FindClose(h);return result;
#else
    DIR *dir;struct dirent *entry;struct stat st;int result=1;
    if(lstat(path,&st)||!S_ISDIR(st.st_mode)||S_ISLNK(st.st_mode))return 0;
    dir=opendir(path);if(!dir)return 0;
    for(;;){char full[FS_PATH];unsigned flags=0;errno=0;entry=readdir(dir);if(!entry){if(errno)result=0;break;}
        if(ps_text_equal(entry->d_name,".")||ps_text_equal(entry->d_name,".."))continue;
        if(!fs_join(full,path,entry->d_name)||lstat(full,&st)){result=0;break;}
        if(S_ISDIR(st.st_mode))flags|=PS_FILE_DIRECTORY;
        if(S_ISLNK(st.st_mode))flags|=PS_FILE_LINK|PS_FILE_DISABLED;
        if(!emit(ctx,entry->d_name,flags)){result=2;break;}
    }
    closedir(dir);return result;
#endif
}
static int fs_absolute(char *out,const char *path){
#ifdef _WIN32
    WCHAR input[FS_PATH],full[FS_PATH];DWORD n;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,input,FS_PATH))return 0;
    n=GetFullPathNameW(input,FS_PATH,full,0);if(!n||n>=FS_PATH)return 0;
    return WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,full,-1,out,FS_PATH,0,0)!=0;
#else
    char cwd[FS_PATH];if(path[0]=='/')return fs_copy(out,FS_PATH,path);
    return getcwd(cwd,sizeof cwd)&&fs_join(out,cwd,path);
#endif
}
typedef struct {ps_file_node nodes[128];char names[128][FS_NAME],paths[128][FS_PATH];unsigned count,parent,error;} NativeFiles;
static NativeFiles native_files;
static int fs_append(void *ctx,const char *name,unsigned flags){NativeFiles *f=ctx;unsigned n=f->count;
    if(n==128)return 0;
    if(!fs_copy(f->names[n],FS_NAME,name)||!fs_join(f->paths[n],f->paths[f->parent],name)){f->error=1;return 0;}
    f->nodes[n].name=f->names[n];f->nodes[n].parent=f->parent;f->nodes[n].flags=flags;++f->count;return 1;
}
static int fs_order(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
static unsigned fs_load(void *ctx,unsigned parent,unsigned count){
    NativeFiles *f=ctx;unsigned i,j,k;int result;f->parent=parent;f->count=count;f->error=0;
    result=fs_scan(f->paths[parent],fs_append,f);if(!result||f->error){f->count=count;return 0;}
    /* Sort only new siblings; existing stable node indices never change. */
    for(i=count;i<f->count;++i)for(j=i+1;j<f->count;++j){
        unsigned di=!!(f->nodes[i].flags&PS_FILE_DIRECTORY),dj=!!(f->nodes[j].flags&PS_FILE_DIRECTORY);
        if(di<dj||(di==dj&&fs_order(f->names[i],f->names[j])>0)){
            unsigned flags=f->nodes[i].flags;f->nodes[i].flags=f->nodes[j].flags;f->nodes[j].flags=flags;
            for(k=0;k<FS_NAME;++k){char c=f->names[i][k];f->names[i][k]=f->names[j][k];f->names[j][k]=c;}
            for(k=0;k<FS_PATH;++k){char c=f->paths[i][k];f->paths[i][k]=f->paths[j][k];f->paths[j][k]=c;}
        }
    }
    return f->count;
}
static void fs_pick(ps_session *s,void *ctx,unsigned id){NativeFiles *f=ctx;if(id!=PS_FILE_ROOT){ps_write(s,"Selected: ");ps_write(s,f->paths[id]);ps_newline(s);}}
static int fs_print(void *ctx,const char *name,unsigned flags){ps_session *s=ctx;ps_write(s,flags&PS_FILE_LINK?"link  ":flags&PS_FILE_DIRECTORY?"dir   ":"file  ");ps_write(s,name);ps_newline(s);return 1;}
static int fs_command(ps_session *s,int argc,const char *const *argv,void *ctx){
    NativeFiles *f=ctx;const char *path=".";unsigned layout=PS_FILE_SPLIT,icons=PS_FILE_ICONS_UNICODE,i;int listing=ps_text_equal(argv[0],"ls"),have_path=0;
    for(i=1;i<(unsigned)argc;++i){
        if(ps_text_equal(argv[i],"--tree"))layout=PS_FILE_TREE;
        else if(ps_text_equal(argv[i],"--list"))layout=PS_FILE_LIST;
        else if(ps_text_equal(argv[i],"--split"))layout=PS_FILE_SPLIT;
        else if(ps_text_equal(argv[i],"--nerd"))icons=PS_FILE_ICONS_NERD;
        else if(ps_text_equal(argv[i],"--emoji"))icons=PS_FILE_ICONS_EMOJI;
        else if(ps_text_equal(argv[i],"--ascii"))icons=PS_FILE_ICONS_ASCII;
        else if(!have_path){path=argv[i];have_path=1;}else return 2;
    }
    /* Do not mutate a borrowed provider snapshot while another picker owns it. */
    if(ps_file_picker_active(s)){ps_write(s,"Close the current browser first");ps_newline(s);return 2;}
    if(!fs_absolute(f->paths[0],path)){ps_write(s,"Invalid or oversized path");ps_newline(s);return 2;}
    if(listing){int ok=fs_scan(f->paths[0],fs_print,s);if(!ok){ps_write(s,"Directory unavailable");ps_newline(s);}return ok?0:1;}
    fs_copy(f->names[0],FS_NAME,"Root");f->nodes[0].name=f->names[0];f->nodes[0].parent=PS_FILE_ROOT;f->nodes[0].flags=PS_FILE_DIRECTORY;
    f->count=1;
    ps_file_picker_style(s,layout,icons);
    return ps_file_browser_open(s,f->paths[0],f->nodes,1,fs_pick,fs_load,f)?0:2;
}
static int fs_attach(ps_session *s){
    ps_command files={"files","[--tree|--list|--split] [--nerd|--emoji|--ascii] [PATH]","Files",fs_command,0,&native_files};
    ps_command list={"ls","[PATH]","Files",fs_command,0,&native_files};
    return ps_register(s,&files)||ps_register(s,&list);
}
#endif
