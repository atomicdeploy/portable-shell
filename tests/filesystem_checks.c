#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "../host/filesystem.h"
#include <assert.h>
#include <string.h>
static union {void *align;unsigned char bytes[PS_DEFAULT_SESSION_STORAGE];} memory;
static char output[262144];static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;while(n--&&used+1<sizeof output)output[used++]=(char)*p++;output[used]=0;}
static void clear_output(void){used=0;output[0]=0;}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,24,PS_UNICODE};ps_session *s;
    const char *list[]={"ls",argc>1?argv[1]:"."};
    const char *files[]={"files","--split",argc>1?argv[1]:"."};
    assert(argc==2);s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(s&&!fs_attach(s));
    assert(!ps_execute_argv(s,2,list));assert(strstr(output,"Documents")&&strstr(output,"README.txt")&&(strstr(output,"Caf\xc3\xa9.txt")||strstr(output,"Caf?.txt"))&&!strchr(output,27));
    clear_output();assert(ps_execute_argv(s,3,files)==2&&!ps_file_picker_active(s));
    cfg.flags=PS_INTERACTIVE|PS_COLOR|PS_UNICODE;s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(s&&!fs_attach(s));
    assert(!ps_execute_argv(s,3,files));assert(ps_file_picker_active(s)==5);assert(ps_text_equal(native_files.names[3],"Caf\xc3\xa9.txt"));
    ps_key(s,PS_KEY_DOWN);assert(ps_file_picker_active(s)==7);
    ps_key(s,9);ps_key(s,PS_KEY_END);clear_output();ps_key(s,13);
    assert(!ps_file_picker_active(s)&&strstr(output,"Selected:")&&strstr(output,"notes.txt"));
    files[1]="--list";assert(!ps_execute_argv(s,3,files));ps_key(s,13);clear_output();ps_key(s,8);assert(strstr(output,"README.txt"));ps_key(s,27);
    files[1]="--emoji";assert(!ps_execute_argv(s,3,files));clear_output();ps_resize(s,30,12);assert(strstr(output,"Documents"));ps_key(s,27);
    list[1]="missing-directory-for-provider-test";clear_output();assert(ps_execute_argv(s,2,list)==1&&strstr(output,"Directory unavailable"));
    return 0;
}
