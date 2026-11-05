#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
/* Darwin exposes O_NOFOLLOW with its native file-operation extensions. */
#define _DARWIN_C_SOURCE 1
#endif
#endif
#include "env_file.h"
#include "portable_util.h"
#ifdef _WIN32
/* Replay fixture dimensions are not Windows SDK identifiers. */
#ifdef W
#undef W
#endif
#ifdef H
#undef H
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static int wide_path(const char *path,WCHAR *wide){return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,PS_ENV_PATH)!=0;}
int env_file_create(const char *path,const char **error){
    WCHAR wide[PS_ENV_PATH];HANDLE file;*error="Cannot create file (existing paths are never overwritten)";
    if(!wide_path(path,wide))return 0;
    file=CreateFileW(wide,GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!CloseHandle(file))return 0;
    *error=0;return 1;
}
int env_file_read(const char *path,char *bytes,unsigned *size,const char **error){
    WCHAR wide[PS_ENV_PATH];HANDLE file;BY_HANDLE_FILE_INFORMATION info;DWORD n,total=0;int ok=0;
    *error="Cannot read a regular UTF-8 path";
    if(!wide_path(path,wide))return 0;
    file=CreateFileW(wide,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,0);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(GetFileType(file)!=FILE_TYPE_DISK||!GetFileInformationByHandle(file,&info)||
       (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||info.nFileSizeHigh||info.nFileSizeLow>=PS_ENV_BYTES)goto done;
    while(total<PS_ENV_BYTES){
        if(!ReadFile(file,bytes+total,PS_ENV_BYTES-total,&n,0))goto done;
        if(!n)break;
        total+=n;
    }
    if(total<PS_ENV_BYTES){bytes[total]=0;*size=total;ok=1;*error=0;}
done:CloseHandle(file);return ok;
}
int env_file_save(const char *path,env_document *d,const char **error){
    WCHAR wide[PS_ENV_PATH],temp[PS_ENV_PATH];HANDLE file;unsigned i,n;DWORD wrote,total=0,security_size=0;int ok=0;
    void *security;SECURITY_ATTRIBUTES attributes;
    *error="Cannot save document";
    if(!env_file_read(path,d->scratch,&n,error))return 0;
    if(n!=d->saved_size){*error="File changed on disk; reload before saving";return 0;}
    for(i=0;i<n;++i)if(d->scratch[i]!=d->saved[i]){*error="File changed on disk; reload before saving";return 0;}
    if(!wide_path(path,wide))return 0;
    for(i=0;wide[i];++i)temp[i]=wide[i];
    /* Exclusive sibling: never follow or overwrite a pre-existing temporary file. */
    {const WCHAR suffix[]=L".portable-shell.tmp";unsigned k=0;
        if(i+PS_COUNT_OF(suffix)>PS_ENV_PATH)return 0;
        while((temp[i++]=suffix[k++])!=0){}
    }
    GetFileSecurityW(wide,DACL_SECURITY_INFORMATION,0,0,&security_size);
    if(!security_size)return 0;
    security=HeapAlloc(GetProcessHeap(),0,security_size);if(!security)return 0;
    if(!GetFileSecurityW(wide,DACL_SECURITY_INFORMATION,security,security_size,&security_size)){HeapFree(GetProcessHeap(),0,security);return 0;}
    attributes.nLength=sizeof attributes;attributes.lpSecurityDescriptor=security;attributes.bInheritHandle=FALSE;
    file=CreateFileW(temp,GENERIC_WRITE,0,&attributes,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
    HeapFree(GetProcessHeap(),0,security);
    if(file==INVALID_HANDLE_VALUE){*error="Cannot create save staging file (it may already exist)";return 0;}
    while(total<d->size){if(!WriteFile(file,d->text+total,d->size-total,&wrote,0)||!wrote)goto done;total+=wrote;}
    if(!FlushFileBuffers(file))goto done;
    ok=1;
done:if(!CloseHandle(file))ok=0;
    if(ok){
        /* Recheck after staging; ReplaceFile preserves the original ACL and metadata. */
        ok=env_file_read(path,d->scratch,&n,error)&&n==d->saved_size;
        for(i=0;ok&&i<n;++i)if(d->scratch[i]!=d->saved[i])ok=0;
        if(ok)ok=ReplaceFileW(wide,temp,0,0,0,0)!=0;
    }
    if(!ok){DeleteFileW(temp);*error="Save failed or file changed; original retained, draft remains open";return 0;}
    env_mark_saved(d);*error=0;return 1;
}
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h> /* rename */
int env_file_create(const char *path,const char **error){
    int fd;*error="Cannot create file (existing paths are never overwritten)";
    fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return 0;
    if(close(fd))return 0;
    *error=0;return 1;
}
int env_file_read(const char *path,char *bytes,unsigned *size,const char **error){
    int fd;struct stat st;unsigned total=0;ssize_t n;
    *error="Cannot read regular file (links and oversized files are refused)";
    fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);if(fd<0)return 0;
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size>=PS_ENV_BYTES){close(fd);return 0;}
    while(total<PS_ENV_BYTES){n=read(fd,bytes+total,PS_ENV_BYTES-total);if(n<0){if(errno==EINTR)continue;close(fd);return 0;}if(!n)break;total+=(unsigned)n;}
    close(fd);if(total==PS_ENV_BYTES)return 0;bytes[total]=0;*size=total;*error=0;return 1;
}
int env_file_save(const char *path,env_document *d,const char **error){
    char temp[PS_ENV_PATH];struct stat st;unsigned i,n,total=0;int fd,ok=1;ssize_t wrote;
    if(!env_file_read(path,d->scratch,&n,error))return 0;
    *error="File changed on disk; reload before saving";
    if(n!=d->saved_size)return 0;
    for(i=0;i<n;++i)if(d->scratch[i]!=d->saved[i])return 0;
    if(lstat(path,&st)||!S_ISREG(st.st_mode))return 0;
    n=env_length(path);if(n+sizeof ".portable-shell.tmp">sizeof temp)return 0;
    env_copy(temp,path,n);env_copy(temp+n,".portable-shell.tmp",sizeof ".portable-shell.tmp");
    fd=open(temp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    if(fd<0){*error="Cannot create private staging file (it may already exist)";return 0;}
    while(total<d->size){wrote=write(fd,d->text+total,d->size-total);if(wrote<0&&errno==EINTR)continue;if(wrote<=0){ok=0;break;}total+=(unsigned)wrote;}
    /* Refuse to silently change the owner/group of a file being replaced. */
    if(ok&&(fchown(fd,st.st_uid,st.st_gid)||fchmod(fd,st.st_mode&0777)||fsync(fd)))ok=0;
    if(close(fd))ok=0;
    if(ok){ok=env_file_read(path,d->scratch,&n,error)&&n==d->saved_size;
        for(i=0;ok&&i<n;++i)if(d->scratch[i]!=d->saved[i])ok=0;
        if(ok)ok=rename(temp,path)==0;}
    if(!ok){unlink(temp);*error="Save failed or file changed; original retained, draft remains open";return 0;}
    env_mark_saved(d);*error=0;return 1;
}
#endif
