#include "portable_shell.h"
#include "portable_style.h"
#include <assert.h>
#include <string.h>
static union {void *align;unsigned char bytes[40000];} memory;
static char output[131072];static unsigned used;
static void sink(void *context,const ps_byte *p,unsigned n){(void)context;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;}
static void clear(void){used=0;output[0]=0;}
static void frame(int cleared){
    assert(!!strstr(output,"\033[2J")==cleared);
    if(PS_SYNC_OUTPUT){
        const char *begin=strstr(output,"\033[?2026h"),*end=strstr(output,"\033[?2026l");
        assert(begin&&end&&begin<end&&!strstr(end+8,"\033[?2026h"));
        assert(!cleared||begin<strstr(output,"\033[2J"));
    }
}
int main(void){
    ps_config config={sink,0,80,24,PS_INTERACTIVE|PS_UNICODE|PS_COLOR};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&config);
    const ps_file_node nodes[]={{"Root",PS_FILE_ROOT,PS_FILE_DIRECTORY},{"one.txt",0,0},{"two.txt",0,0}};
    assert(s);ps_start(s);clear();ps_key(s,'x');frame(0);assert(!strstr(output,"\033[2K"));ps_key(s,3);
    ps_execute(s,"menu");clear();ps_key(s,PS_KEY_DOWN);frame(0);ps_menu_close(s);
    assert(ps_file_picker_open(s,"Files",nodes,3,0,0));clear();ps_key(s,PS_KEY_DOWN);frame(0);
    clear();ps_resize(s,90,25);frame(1);ps_file_picker_close(s);
#if PS_EDITOR
    assert(ps_editor_open(s,"Notes","abc\ndef",0,0));clear();ps_key(s,PS_KEY_END);frame(0);
    clear();ps_key(s,8);frame(0);assert(strstr(output,"ab "));
    ps_key(s,27);ps_key(s,'d');
#endif
    ps_execute(s,"controls");ps_tick(s,1000);clear();ps_key(s,PS_KEY_DOWN);ps_tick(s,1100);frame(0);ps_close(s);
    config.flags=PS_INTERACTIVE|PS_NO_SYNC_OUTPUT;s=ps_init(memory.bytes,sizeof memory.bytes,&config);
    clear();ps_execute(s,"menu");ps_key(s,PS_KEY_DOWN);assert(!strstr(output,"2026"));ps_close(s);
    config.flags=0;s=ps_init(memory.bytes,sizeof memory.bytes,&config);clear();ps_execute(s,"echo plain");assert(!strchr(output,27));
    return 0;
}
