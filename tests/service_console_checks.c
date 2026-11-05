/* Real core byte-stream fixtures, not hand-drawn terminal screenshots. */
#include "../examples/service_console_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
enum { PREVIEW_COLUMNS=100, PREVIEW_ROWS=34, SVG_CAPACITY=800000 };
static cw_workspace workspace;
static FILE *capture;
static char svg[SVG_CAPACITY];
static unsigned bytes_written;
static void sink(void *context,const ps_byte *bytes,unsigned count){
    (void)context;bytes_written+=count;
    if(capture)assert(fwrite(bytes,1,count,capture)==count);
    ps_replay_feed(bytes,count);
}
static int save_svg(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int failed;
    if(!f||n>=sizeof svg)return 0;
    failed=fwrite(svg,1,n,f)!=n;return fclose(f)==0&&!failed;
}
static unsigned screen_index(const char *id){unsigned i;for(i=0;i<PS_COUNT_OF(service_console_screens);++i)if(!strcmp(service_console_screens[i].id,id))return i;abort();}
static void open_screen(ps_session *s,const char *id){
    (void)s;workspace.notice[0]=0;
    assert(cw_open(&workspace,id));
    if(!strcmp(id,"config-inline"))cw_key(&workspace,13);
    if(!strcmp(id,"config-section")){cw_key(&workspace,PS_KEY_UP);cw_key(&workspace,13);}
}
static void tests(ps_session *s){
    unsigned i,revision;char before[PS_EDITOR_CAPACITY];
#if PS_FORM
    unsigned accepted;
#endif
    /* Every screen opens; every menu target resolves; every viewport responds. */
    for(i=0;i<PS_COUNT_OF(service_console_screens);++i){const cw_screen *screen=&service_console_screens[i];unsigned j;
        if((screen->kind==CW_FORM&&!PS_FORM)||(screen->kind==CW_DOCUMENT&&!PS_EDITOR)){
            unsigned current=workspace.current;assert(!cw_open(&workspace,screen->id));assert(workspace.current==current);continue;
        }
        assert(cw_open(&workspace,screen->id));
        ps_resize(s,64,20);ps_resize(s,PREVIEW_COLUMNS,PREVIEW_ROWS);
        if(screen->kind==CW_MENU){
            cw_key(&workspace,PS_KEY_END);cw_key(&workspace,PS_KEY_HOME);
            for(j=0;j<screen->count;++j)if(screen->entries[j].target&&*screen->entries[j].target){
                assert(cw_open(&workspace,screen->id));
                assert(ps_menu_choose(s,j+1)!=2);cw_dispatch(&workspace);
                {unsigned target=screen_index(screen->entries[j].target);const cw_screen *dest=&service_console_screens[target];
                 assert(workspace.current==(((dest->kind==CW_FORM&&!PS_FORM)||(dest->kind==CW_DOCUMENT&&!PS_EDITOR))?i:target));}
            }
        }else if(screen->kind==CW_TABLE){cw_key(&workspace,PS_KEY_PAGEDOWN);cw_key(&workspace,PS_KEY_PAGEUP);}
        else if(screen->kind==CW_DOCUMENT){
            snprintf(before,sizeof before,"%s",ps_editor_text(s));
            cw_feed(&workspace,"must not insert",PS_LITERAL_LENGTH("must not insert"));
            cw_feed(&workspace,"\033[200~paste\033[201~",PS_LITERAL_LENGTH("\033[200~paste\033[201~"));
            cw_key(&workspace,127);cw_key(&workspace,PS_KEY_DELETE);cw_key(&workspace,19);cw_key(&workspace,26);
            assert(!strcmp(before,ps_editor_text(s)));
            cw_key(&workspace,PS_KEY_END);cw_key(&workspace,PS_KEY_HOME);
            cw_key(&workspace,PS_KEY_PAGEDOWN);cw_key(&workspace,PS_KEY_PAGEUP);
        }
    }
#if PS_FORM
    assert(cw_open(&workspace,"config-edit"));accepted=workspace.accepted;
    cw_key(&workspace,13);cw_key(&workspace,21);cw_feed(&workspace,"http://localhost:9000",PS_LITERAL_LENGTH("http://localhost:9000"));cw_key(&workspace,13);
    assert(workspace.accepted==accepted+1&&workspace.current==screen_index("configuration"));
    i=workspace.slots[workspace.offsets[screen_index("configuration")]];
    assert(!strcmp(workspace.values[i],"http://localhost:9000"));
    assert(cw_open(&workspace,"config-edit"));cw_key(&workspace,13);cw_key(&workspace,21);cw_feed(&workspace,"discard me",PS_LITERAL_LENGTH("discard me"));cw_key(&workspace,27);
    assert(!strcmp(workspace.values[i],"http://localhost:9000")&&workspace.accepted==accepted+1);
    assert(cw_open(&workspace,"diag-port"));cw_key(&workspace,13);cw_key(&workspace,21);cw_feed(&workspace,"999999",6);cw_key(&workspace,13);assert(ps_form_active(s));cw_key(&workspace,27);
    assert(cw_open(&workspace,"diag-port"));cw_key(&workspace,9);assert(ps_form_active(s)==2);
    cw_key(&workspace,PS_KEY_BACKTAB);assert(ps_form_active(s)==1);cw_key(&workspace,27);
#endif
    assert(cw_open(&workspace,"settings"));revision=workspace.revision;
    cw_key(&workspace,PS_KEY_DOWN);cw_key(&workspace,PS_KEY_RIGHT);assert(workspace.revision>revision);
    assert(cw_open(&workspace,"phone-tab-0"));assert(workspace.panel.count==6);
    for(i=0;i<6;++i){cw_key(&workspace,9);assert(ps_panel_active(s)==(i+1)%6+1);}
#if PS_FORM
    assert(cw_open(&workspace,"phone-tab-2"));accepted=workspace.requests;
    cw_key(&workspace,13);cw_key(&workspace,13);assert(workspace.requests==accepted); /* Cancel is initially focused. */
    cw_key(&workspace,13);cw_key(&workspace,9);cw_key(&workspace,13);
    assert(workspace.requests==accepted+1&&ps_form_active(s)&&workspace.current==screen_index("phone-provision"));
    cw_key(&workspace,27);
#endif
    assert(cw_open(&workspace,"configuration"));cw_key(&workspace,13);cw_key(&workspace,27);
    assert(ps_panel_active(s));cw_key(&workspace,27);
    assert(!ps_panel_active(s)&&workspace.current==screen_index("main"));
    assert(cw_open(&workspace,"configuration"));cw_feed(&workspace,"\033",1);cw_tick(&workspace,1000);
    assert(!ps_panel_active(s)&&workspace.current==screen_index("main"));
    printf("95 screens: open, resize, navigation; menu routes; read-only paste/save guards; form validation/commit/cancel; shared settings; six tabs passed.\n");
}
int main(int argc,char **argv){
    ps_config cfg={sink,0,PREVIEW_COLUMNS,PREVIEW_ROWS,PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_TRUECOLOR};
    void *storage=malloc(ps_sizeof_session());ps_session *s;unsigned i;
    assert(storage);s=ps_init(storage,ps_sizeof_session(),&cfg);assert(s);
    assert(cw_init(&workspace,s,service_console_screens,PS_COUNT_OF(service_console_screens)));
    tests(s);
    if(argc==2){
        ps_close(s);s=ps_init(storage,ps_sizeof_session(),&cfg);assert(s);
        assert(cw_init(&workspace,s,service_console_screens,PS_COUNT_OF(service_console_screens)));
        for(i=0;i<PS_COUNT_OF(service_console_screens);++i){char path[1024];const char *id=service_console_screens[i].id;
            ps_close(s);s=ps_init(storage,ps_sizeof_session(),&cfg);assert(s);
            assert(cw_init(&workspace,s,service_console_screens,PS_COUNT_OF(service_console_screens)));
            snprintf(path,sizeof path,"%s/%s.ansi",argv[1],id);capture=fopen(path,"wb");assert(capture);
            ps_replay_reset();bytes_written=0;open_screen(s,id);ps_resize(s,PREVIEW_COLUMNS,PREVIEW_ROWS);
            assert(bytes_written);assert(fclose(capture)==0);capture=0;
            snprintf(path,sizeof path,"%s/%s.svg",argv[1],id);assert(save_svg(path));
        }
    }
    printf("Session storage: %u bytes; consumer model: %u bytes; catalog screens: %u\n",(unsigned)ps_sizeof_session(),(unsigned)sizeof workspace,(unsigned)PS_COUNT_OF(service_console_screens));
    free(storage);return 0;
}
