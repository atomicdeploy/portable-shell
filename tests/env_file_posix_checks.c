#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int save_checkpoint(int fd);
/* Exercise the real adapter with deterministic entry replacement immediately
   after staging. Only fsync is wrapped; all file operations use the real OS. */
#define fsync save_checkpoint
#include "../host/env_file.c"
#undef fsync

static env_document document;
static unsigned mutation;
static const char *error;
static void put(const char *path,const char *text){
    int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);size_t n=strlen(text);
    assert(fd>=0&&write(fd,text,n)==(ssize_t)n);assert(!close(fd));
}
static void expect(const char *path,const char *text){
    char bytes[PS_ENV_BYTES];unsigned n;
    assert(env_file_read(path,bytes,&n,&error));assert(n==strlen(text)&&!strcmp(bytes,text));
}
static void draft(const char *path){
    unsigned n;assert(env_file_read(path,document.scratch,&n,&error));
    assert(env_load(&document,document.scratch,n));assert(env_set_value(&document,0,"two"));
}
static int save_checkpoint(int fd){
    int result=fsync(fd);
    if(!result&&mutation==1){
        mutation=0;assert(!rename("values.env","original.env"));put("values.env","KEY=one\n");
    }else if(!result&&mutation==2){
        mutation=0;assert(!rename("parent","moved"));assert(!mkdir("parent",0700));
        put("parent/values.env","KEY=imposter\n");
    }
    return result;
}
int main(void){
    char sandbox[]="/tmp/portable-shell-env-XXXXXX",previous[4096];int fd;struct stat before,after;
    assert(getcwd(previous,sizeof previous));assert(mkdtemp(sandbox));assert(!chdir(sandbox));
    put("values.env","KEY=one\n");fd=open("values.env",O_RDONLY);assert(fd>=0);
    assert(!fchmod(fd,0640)&&!fstat(fd,&before)&&!close(fd));draft("values.env");
    assert(env_file_save("values.env",&document,&error)&&!env_dirty(&document));expect("values.env","KEY=two\n");
    fd=open("values.env",O_RDONLY);assert(fd>=0&&!fstat(fd,&after)&&!close(fd));
    assert(before.st_uid==after.st_uid&&before.st_gid==after.st_gid&&(after.st_mode&0777)==0640);
    put("values.env","KEY=one\n");draft("values.env");put("values.env.portable-shell.tmp","occupied\n");
    assert(!env_file_save("values.env",&document,&error)&&env_dirty(&document));
    expect("values.env.portable-shell.tmp","occupied\n");expect("values.env","KEY=one\n");
    assert(!unlink("values.env.portable-shell.tmp"));
    assert(!symlink("values.env","values.env.portable-shell.tmp"));
    assert(!env_file_save("values.env",&document,&error));expect("values.env","KEY=one\n");
    assert(!unlink("values.env.portable-shell.tmp"));
    assert(!symlink("values.env","link.env"));
    {char bytes[PS_ENV_BYTES];unsigned n;
        assert(!env_file_read("link.env",bytes,&n,&error));
        assert(!env_file_save("link.env",&document,&error));
        assert(!mkfifo("pipe.env",0600));assert(!env_file_read("pipe.env",bytes,&n,&error));
        assert(!unlink("pipe.env"));
    }
    assert(!unlink("link.env"));
    fd=open(".",O_RDONLY|O_DIRECTORY);assert(fd>=0&&!fchmod(fd,0770));
    assert(!env_file_save("values.env",&document,&error)&&env_dirty(&document));
    assert(strstr(error,"directory"));expect("values.env","KEY=one\n");assert(!fchmod(fd,0700)&&!close(fd));
    mutation=1;assert(!env_file_save("values.env",&document,&error)&&env_dirty(&document));
    expect("values.env","KEY=one\n");expect("original.env","KEY=one\n");assert(!unlink("original.env"));
    /* The same bytes on a new inode are a conflict, never permission metadata
       borrowed from one file and used to replace another. */
    assert(env_file_create("values.env.portable-shell.tmp",&error));assert(!unlink("values.env.portable-shell.tmp"));
    assert(!mkdir("parent",0700));put("parent/values.env","KEY=one\n");draft("parent/values.env");
    mutation=2;assert(env_file_save("parent/values.env",&document,&error)&&!env_dirty(&document));
    expect("moved/values.env","KEY=two\n");expect("parent/values.env","KEY=imposter\n");
    assert(!unlink("values.env")&&!unlink("moved/values.env")&&!unlink("parent/values.env"));
    assert(!rmdir("moved")&&!rmdir("parent"));assert(!chdir(previous)&&!rmdir(sandbox));
    puts("POSIX env save: descriptor metadata, private staging, symlink/FIFO refusal, directory policy, inode and parent swaps passed");return 0;
}
