/* Native adapter replay: real on-disk enumeration, deterministic fixture in CI. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "../host/filesystem.h"
#include <stdio.h>
void ps_replay_reset(void);void ps_replay_feed(const unsigned char *,unsigned);unsigned ps_replay_svg(char *);
static union{void *align;unsigned char bytes[PS_DEFAULT_SESSION_STORAGE];} memory;
static char svg[400000];
static void output(void *ctx,const ps_byte *p,unsigned n){(void)ctx;ps_replay_feed(p,n);}
static int save(const char *path){unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;if(!f)return 1;bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;}
int main(int argc,char **argv){
    ps_config cfg={output,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_UNICODE};ps_session *s;unsigned i;
    const unsigned layouts[]={PS_FILE_TREE,PS_FILE_LIST,PS_FILE_SPLIT,PS_FILE_SPLIT,PS_FILE_SPLIT};
    const unsigned icons[]={PS_FILE_ICONS_UNICODE,PS_FILE_ICONS_UNICODE,PS_FILE_ICONS_UNICODE,PS_FILE_ICONS_NERD,PS_FILE_ICONS_EMOJI};
    if(argc!=7)return 2;
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);if(!s||fs_attach(s))return 3;
    for(i=0;i<5;++i){
        ps_file_picker_close(s);ps_replay_reset();if(!fs_absolute(native_files.paths[0],argv[1]))return 4;
        fs_copy(native_files.names[0],FS_NAME,"Workspace");native_files.nodes[0].name=native_files.names[0];native_files.nodes[0].parent=PS_FILE_ROOT;native_files.nodes[0].flags=PS_FILE_DIRECTORY;
        ps_file_picker_style(s,layouts[i],icons[i]);
        if(!ps_file_browser_open(s,"Files / native directory",native_files.nodes,1,fs_pick,fs_load,&native_files))return 5;
        if(i==0){ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_RIGHT);ps_key(s,PS_KEY_DOWN);}
        if(i>=2){ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_RIGHT);ps_key(s,9);}
        if(save(argv[i+2]))return 6;
    }return 0;
}
