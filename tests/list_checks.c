#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
static char output[262144],svg[800000];static unsigned used,chosen;
static union {void *align;unsigned char bytes[50000];} memory;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);
}
static int action(ps_session *s,void *ctx){(void)s;chosen=*(unsigned *)ctx;return 0;}
static const char *description(void *ctx,const ps_menu *m,unsigned i){
    static const char *const text[]={"Browse and edit device values","Inspect transport health","Edit session documents","Browse directory trees",
        "Review scheduled work","Control output levels","Manage saved preferences","Inspect diagnostic records"};
    (void)ctx;(void)m;return text[i%8];
}
static void keys(ps_session *s,const char *p){used=0;ps_feed(s,p,(unsigned)strlen(p));}
static int save(const char *path){unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;if(!f)return 1;bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;}
int main(int argc,char **argv){
    unsigned ids[]={1,2,3,4,5,6,7,8};
    ps_menu_item items[]={{"Device settings",0,action,&ids[0],0},{"Connection status",0,action,&ids[1],0},
        {"Session notes",0,action,&ids[2],0},{"Files and folders",0,action,&ids[3],0},
        {"Tasks",0,action,&ids[4],0},{"Output controls",0,action,&ids[5],0},
        {"Preferences",0,action,&ids[6],0},{"Diagnostics",0,action,&ids[7],PS_MENU_DISABLED}};
    const ps_menu menu={"Workspace",items,8};
    ps_config cfg={sink,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_TRUECOLOR};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    ps_menu_set_style(s,PS_MENU_LIST);ps_menu_set_description(s,description,0);
    assert(ps_menu_open(s,&menu));
#if PS_MENU_LIST_VIEW
    assert(strstr(output,"Page 1 / 2"));
    keys(s,"\033[6~");assert(strstr(output,"Page 2 / 2")&&!chosen);
    keys(s,"/dvs");assert(strstr(output,"Device settings"));
    keys(s,"\r\r");assert(chosen==1&&!ps_menu_active(s));
    chosen=0;ps_menu_open(s,&menu);
    keys(s,"/zzzzz");assert(strstr(output,"No matching items"));
    keys(s,"\r\r");assert(!chosen&&ps_menu_active(s));
    ps_key(s,27);keys(s,"/diagn\r\r");assert(!chosen&&ps_menu_active(s));
    ps_key(s,27);keys(s,"/transport\r\r");assert(chosen==2);
    chosen=0;ps_menu_open(s,&menu);
#if PS_MOUSE
    keys(s,"\033[<0;10;8M\r");assert(chosen==2);ps_menu_open(s,&menu);
#endif
    keys(s,"/abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    ps_resize(s,24,12);ps_key(s,PS_KEY_LEFT);ps_key(s,PS_KEY_DELETE);ps_key(s,27);
    ps_resize(s,96,24);ps_menu_close(s);
#if PS_MENU_DEPTH > 1
    {
        ps_menu_item entry={"Submenu",&menu,0,0,0};ps_menu parent={"Parent",&entry,1};
        ps_menu_open(s,&parent);keys(s,"\r");assert(ps_menu_active(s)==2);
        ps_key(s,27);assert(ps_menu_active(s)==1);ps_menu_close(s);
    }
#endif
#else
    (void)keys;
    assert(strstr(output,"Device settings"));
#endif
    if(argc==1)return 0;
    if(argc!=3)return 2;
    ps_replay_reset();ps_menu_open(s,&menu);keys(s,"\033[B?");
    if(save(argv[1]))return 3;
    keys(s,"/device");assert(strcmp(ps_replay_cell(7,4)," ")==0);return save(argv[2]);
}
