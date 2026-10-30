/* Emitted-byte fixtures; file nodes represent a simulated provider, not disk. */
#include "portable_shell.h"
#include <stdio.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static union {void *align;unsigned char bytes[40000];} memory;
static char svg[400000];
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;ps_replay_feed(p,n);}
static int action(ps_session *s,void *ctx){(void)s;(void)ctx;return 0;}
static const char *description(void *ctx,const ps_menu *m,unsigned i){
    static const char *const details[]={"Review current device settings.","Inspect connection and transport status.","Edit the session document.","Browse available files and folders."};
    (void)ctx;(void)m;return details[i];
}
static int save(const char *path){unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;if(!f)return 1;bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_UNICODE};ps_session *s;
    static const ps_menu_item items[]={ {"Device settings",0,action,0,0},{"Connection status",0,action,0,0},{"Session notes",0,action,0,0},{"Files and folders",0,action,0,0} };
    static const ps_menu menu={"Workspace",items,4};
    static const ps_file_node nodes[]={ {"Workspace",PS_FILE_ROOT,PS_FILE_DIRECTORY},{"Documents",0,PS_FILE_DIRECTORY},{"notes.txt",1,0},{"release-checklist.txt",1,0},{"Configuration",0,PS_FILE_DIRECTORY},{"device.conf",4,0},{"README.txt",0,0} };
    if(argc!=5)return 2;
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);ps_replay_reset();ps_menu_set_style(s,PS_MENU_TABLE);ps_menu_open(s,&menu);ps_key(s,PS_KEY_DOWN);if(save(argv[1]))return 3;
    ps_menu_set_style(s,PS_MENU_TABLE|PS_MENU_NUMBERS|PS_MENU_DETAILS);ps_menu_set_description(s,description,0);ps_menu_open(s,&menu);ps_key(s,PS_KEY_DOWN);if(save(argv[2]))return 4;
    ps_menu_close(s);ps_replay_reset();ps_editor_set_style(s,PS_EDITOR_HEADER|PS_EDITOR_BORDER);
    ps_editor_open(s,"Session notes", "Release checklist\n\nVerify device settings and connection status before applying the configuration.\n\nConnection\n  Hostname: workbench\n  Transport: serial or TCP\n\nAfter saving\n  Review observed values and pending requests.\n",0,0);
    ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_END);if(save(argv[3]))return 5;
    ps_key(s,27);ps_replay_reset();ps_file_picker_open(s,"Files / Workspace",nodes,7,0,0);
    ps_key(s,PS_KEY_RIGHT);ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_RIGHT);ps_key(s,PS_KEY_DOWN);if(save(argv[4]))return 6;
    return 0;
}
