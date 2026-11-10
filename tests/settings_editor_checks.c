#include "../examples/settings_editor_model.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
unsigned ps_replay_row(void);
unsigned ps_replay_column(void);
void ps_replay_set_accent(unsigned);
unsigned ps_replay_foreground(unsigned,unsigned);
enum { TEST_COLUMNS=100,TEST_ROWS=34,SVG_BYTES=1000000 };
static settings_demo model;
static char svg[SVG_BYTES];
static ps_session *session;
static ps_u32 time_now;
static int long_read(void *context,ps_setting_value *v){(void)context;v->text="This deliberately long provider value exceeds the inline editor capacity and must never be truncated on write.";return 1;}
static void output(void *ctx,const ps_byte *bytes,unsigned count){(void)ctx;ps_replay_feed(bytes,count);}
static void key(unsigned k){ps_key(session,k);ps_tick(session,time_now+=100);}
static void type(const char *text){while(*text)key((unsigned char)*text++);}
static void edit(unsigned id,const char *text){assert(ps_panel_select(session,id));key(13);key(21);type(text);key(13);}
static void capture(const char *path){FILE *f;unsigned n;if(!path)return;n=ps_replay_svg(svg);assert(n&&n<sizeof svg);f=fopen(path,"wb");assert(f);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));}
static int visible(const char *text){unsigned x,y;for(y=0;y<TEST_ROWS;++y){char row[TEST_COLUMNS*4+1];unsigned n=0;for(x=0;x<TEST_COLUMNS;++x){const char *p=ps_replay_cell(y,x);while(*p&&n+1<sizeof row)row[n++]=*p++;}row[n]=0;if(strstr(row,text))return 1;}return 0;}
static int text_at(unsigned y,unsigned x,const char *text){
    while(*text&&x<TEST_COLUMNS){const char *cell=ps_replay_cell(y,x++);size_t n=strlen(cell);
        if(!n||strncmp(text,cell,n))return 0;
        text+=n;
    }return !*text;
}
static void locate(const char *text,unsigned *y,unsigned *x){
    for(*y=0;*y<TEST_ROWS;++*y)for(*x=0;*x<TEST_COLUMNS;++*x)if(text_at(*y,*x,text))return;
    assert(!"Expected text missing from terminal replay");
}
int main(int argc,char **argv){
    ps_config config={output,0,TEST_COLUMNS,TEST_ROWS,PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_TRUECOLOR};
    ps_setting_value value;ps_control_value observed,request;void *memory=calloc(1,ps_sizeof_session());
    assert(memory);session=ps_init(memory,ps_sizeof_session(),&config);assert(session);
    assert(settings_demo_init(&model,session));
    assert(!strcmp(model.controls[0].group,"api")&&!strcmp(model.controls[3].group,"application"));
    assert(!ps_settings_editor_init(&model.editor,&model.registry,model.controls,model.rows,SETTINGS_DEMO_COUNT-1,"Too small"));
    {ps_setting d=model.items[0].descriptor;
        d.group="";assert(!ps_settings_editor_group_size(&d));
        d.group="Custom";assert(ps_settings_editor_group_size(&d)==PS_LITERAL_LENGTH("Custom"));
        d.group=0;d.name="plain_key_without_a_group";assert(!ps_settings_editor_group_size(&d));
    }
    assert(ps_execute(session,"config api.port 9000")==0);
    assert(ps_settings_get(&model.registry,"api.port",&value)==0&&value.natural==9000);
    assert(ps_execute(session,"config api.port 8000")==0);
    ps_replay_reset();assert(ps_execute(session,"configure")==0);ps_tick(session,time_now+=100);
    assert(visible(PS_PANEL_PRESENTATION?"Base URL":"Base Url")&&visible("8000"));
    assert(model.editor.panel.status&&!model.editor.panel.status[0]);
#if PS_CONTROL_SECTION_ROWS
    assert(visible("API")&&visible("APPLICATION"));
#endif
    capture(argc>1?argv[1]:0);
#if PS_PANEL_PRESENTATION && PS_PANEL_KEYS
    {const char *p="\033[17~";while(*p)ps_feed(session,p++,1);ps_tick(session,time_now+=100);}
    assert(visible("api.base_url"));
    {unsigned y,x,key_color;locate("api.base_url",&y,&x);key_color=ps_replay_foreground(y,x);
        capture(argc>3?argv[3]:0);key(PS_KEY_F6);assert(visible("Base URL"));locate("Base URL",&y,&x);
        assert(key_color!=ps_replay_foreground(y,x));}
#endif
    /* Enter must not shift the value or its cursor into extra cell padding. */
    {unsigned y,x;locate(model.items[0].text,&y,&x);key(13);
        assert(text_at(y,x,model.items[0].text));
        assert(ps_replay_row()==y&&ps_replay_column()==x+strlen(model.items[0].text));
        capture(argc>2?argv[2]:0);
        key(PS_KEY_HOME);assert(ps_replay_column()==x);
#if PS_MOUSE
        {char sequence[80];int n=snprintf(sequence,sizeof sequence,"\033[<0;%u;%uM\033[<0;%u;%um",x+2,y+1,x+2,y+1);
            assert(n>0&&(unsigned)n<sizeof sequence);ps_feed(session,sequence,(unsigned)n);ps_tick(session,time_now+=100);
            assert(ps_replay_column()==x+1);
        }
#endif
        key(27);assert(text_at(y,x,model.items[0].text));
    }
#if PS_DASHBOARD
    /* The dashboard reuses control_row, so its cursor must share the new origin. */
    assert(ps_panel_set_layout(session,PS_PANEL_GRID));ps_tick(session,time_now+=100);
    {unsigned y,x;locate(model.items[0].text,&y,&x);key(13);
        assert(text_at(y,x,model.items[0].text));
        assert(ps_replay_row()==y&&ps_replay_column()==x+strlen(model.items[0].text));
        key(PS_KEY_HOME);assert(ps_replay_column()==x);key(27);
    }
    assert(ps_panel_set_layout(session,PS_PANEL_TABS));ps_tick(session,time_now+=100);
#endif
    edit(0,"http://localhost:9000");assert(!strcmp(model.items[0].text,"http://localhost:9000"));
    edit(0,"invalid");assert(!strcmp(model.items[0].text,"http://localhost:9000"));key(27);
    edit(1,"65536");assert(model.items[1].value.natural==8000);key(27);
    edit(1,"8080");assert(model.items[1].value.natural==8080);
    assert(ps_panel_select(session,2));key(' ');assert(!model.items[2].value.natural);
    assert(ps_panel_select(session,3));key(PS_KEY_RIGHT);assert(model.items[3].value.natural==1);
#if PS_CONTROL_CHOICE_POPUP
    assert(ps_panel_select(session,11));ps_tick(session,time_now+=100);
    {unsigned value_y,value_x,option_y,option_x;
        locate(PS_PANEL_PRESENTATION?"Default":"default",&value_y,&value_x);key(13);
        locate(PS_PANEL_PRESENTATION?"One Dark":"onedark",&option_y,&option_x);
        assert(option_x==value_x+6&&option_y<value_y); /* value-anchored drop-up */
    }
    assert(visible(PS_PANEL_PRESENTATION?"One Dark":"onedark")&&visible(PS_PANEL_PRESENTATION?"Default":"default"));
    assert(!ps_panel_set_key_names(session,1));
    key(PS_KEY_UP);assert(model.items[11].value.natural==0); /* rollover is a draft */
    capture(argc>4?argv[4]:0);key(27);assert(model.items[11].value.natural==0);
    key(PS_KEY_F2);key(PS_KEY_END);key(13);assert(model.items[11].value.natural==4);
    assert(ps_settings_get(&model.registry,"terminal.palette",&value)==PS_SETTING_OK&&value.natural==4);
    assert(ps_settings_set(&model.registry,"terminal.palette","One Dark")==PS_SETTING_INVALID);
    assert(ps_settings_set(&model.registry,"terminal.palette","default")==PS_SETTING_OK);
    key(13);key(PS_KEY_DOWN);
    assert(ps_settings_set(&model.registry,"terminal.palette","red")==PS_SETTING_OK);
    key(13);assert(model.items[11].value.natural==3);assert(visible("State changed"));
#if PS_MOUSE
    assert(ps_panel_select(session,11));key(13);
    {unsigned y,x;char sequence[48];int n;
        locate(PS_PANEL_PRESENTATION?"One Dark":"onedark",&y,&x);
        n=snprintf(sequence,sizeof sequence,"\033[<0;%u;%uM",x+1,y+1);
        assert(n>0);ps_feed(session,sequence,(unsigned)n);ps_tick(session,time_now+=100);
        assert(model.items[11].value.natural==4);
    }
#endif
    /* Descriptor shape changes invalidate the adapter rather than indexing an old list. */
    assert(ps_panel_select(session,11));key(13);model.items[11].descriptor.choice_count=2;
    key(13);assert(visible("State changed"));model.items[11].descriptor.choice_count=5;
    assert(ps_settings_set(&model.registry,"terminal.palette","default")==PS_SETTING_OK);
    /* Long lists page, scroll and wrap without changing the saved value. */
    {static const char *const choices[]={"00","01","02","03","04","05","06","07","08","09","10","11","12","13","14","15"};
        const char *const *old=model.controls[11].choices;
        model.controls[11].choices=model.items[11].descriptor.choices=choices;
        model.controls[11].choice_count=model.items[11].descriptor.choice_count=PS_COUNT_OF(choices);
        ps_resize(session,48,18);key(13);key(PS_KEY_END);assert(visible("15"));
        key(PS_KEY_PAGEDOWN);key(PS_KEY_HOME);
#if PS_MOUSE
        {const char wheel[]="\033[<64;30;10M";ps_feed(session,wheel,sizeof wheel-1);ps_tick(session,time_now+=100);}
#else
        key(PS_KEY_UP);
#endif
        key(13);assert(model.items[11].value.natural==15);
        key(13);ps_resize(session,3,3);key(PS_KEY_DOWN);key(27);
        model.controls[11].choices=model.items[11].descriptor.choices=old;
        model.controls[11].choice_count=model.items[11].descriptor.choice_count=5;
        assert(ps_settings_set(&model.registry,"terminal.palette","default")==PS_SETTING_OK);
        ps_resize(session,TEST_COLUMNS,TEST_ROWS);
    }
#endif
    assert(ps_panel_select(session,0));key(PS_KEY_HOME);
    ps_set_semantic_accent(session,1,1u<<PS_INK_HEADING);ps_panel_refresh(session);ps_tick(session,time_now+=100);
    ps_replay_set_accent(0x79d99d);capture(argc>5?argv[5]:0);
    ps_replay_set_accent(0xe589ed);capture(argc>6?argv[6]:0);
    ps_set_semantic_accent(session,0,0);ps_panel_refresh(session);ps_tick(session,time_now+=100);
    edit(7,"-42");assert(model.items[7].value.integer==-42);
    edit(8,"4294967295");assert(model.items[8].value.natural==0xffffffffu);
    assert(ps_panel_select(session,0));key(PS_KEY_F2);key(21);type("http://discarded");key(27);
    assert(!strcmp(model.items[0].text,"http://localhost:9000"));
    assert(ps_panel_select(session,9));key(13);assert(ps_panel_selection(session,&observed.number));
    assert(ps_settings_set(&model.registry,"runtime.version","2")==PS_SETTING_READ_ONLY);
    /* CLI/provider change while a draft is open must not be overwritten. */
    assert(ps_panel_select(session,1));key(13);key(21);type("9001");
    assert(ps_settings_set(&model.registry,"api.port","9002")==0);key(13);key(27);
    assert(model.items[1].value.natural==9002);
    assert(ps_settings_editor_read(&model.editor,1,&observed));request=observed;request.number=9010;
    assert(ps_settings_set(&model.registry,"api.port","9020")==0);
    assert(!ps_settings_editor_write(&model.editor,1,&request,observed.revision));
    /* Replacing a descriptor requires rebuilding the table, never stale writes. */
    {const ps_setting *old=model.editor.rows[0].setting;ps_setting replacement=*old;
        assert(ps_settings_register(&model.registry,&replacement)==0);
        assert(!ps_settings_editor_read(&model.editor,0,&observed));
        assert(ps_settings_register(&model.registry,old)==0);
    }
    /* Oversized provider text is locked rather than silently truncated by edit. */
    model.items[0].descriptor.read=long_read;
    assert(ps_settings_editor_read(&model.editor,0,&observed)&&(observed.flags&PS_CONTROL_LOCKED));
    assert(!ps_settings_editor_write(&model.editor,0,&observed,observed.revision));
    model.items[0].descriptor.read=settings_demo_read;
    ps_resize(session,48,18);key(PS_KEY_END);key(PS_KEY_PAGEUP);key(PS_KEY_PAGEDOWN);
    ps_resize(session,TEST_COLUMNS,TEST_ROWS);key(27);assert(!ps_panel_active(session));
    ps_close(session);free(memory);
    puts("Typed registry/table sharing, validation, read-only, full-range integers, stale edits, resize and navigation passed");return 0;
}
