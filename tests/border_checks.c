#include "portable_shell.h"
#include "portable_rich.h"
#include "portable_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { COLUMNS=80, ROWS=24, SVG_BYTES=800000 };
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
unsigned ps_replay_row(void);
unsigned ps_replay_column(void);
static char svg[SVG_BYTES];
static unsigned now,writes,level=64;
static void sink(void *context,const ps_byte *bytes,unsigned size){(void)context;ps_replay_feed(bytes,size);}
static int read_value(void *context,unsigned id,ps_control_value *value){
    (void)context;memset(value,0,sizeof *value);value->flags=PS_CONTROL_VALID;
    if(id==0)value->number=1;
    if(id==1)value->number=level;
    if(id==2)strcpy(value->text,"workbench");
    if(id==3)strcpy(value->text,"Provision");
    return 1;
}
static int write_value(void *context,unsigned id,const ps_control_value *value,ps_u32 revision){
    (void)context;(void)revision;if(id==1)level=value->number;++writes;return 1;
}
static void draw(ps_session *s){ps_tick(s,now+=120);}
static void key(ps_session *s,unsigned k){ps_key(s,k);draw(s);}
static int text_at(unsigned y,unsigned x,const char *text){
    while(*text&&x<COLUMNS){const char *c=ps_replay_cell(y,x++);size_t n=strlen(c);
        if(!n||strncmp(text,c,n))return 0;
        text+=n;}
    return !*text;
}
static void locate(const char *text,unsigned *y,unsigned *x){
    for(*y=0;*y<ROWS;++*y)for(*x=0;*x<COLUMNS;++*x)if(text_at(*y,*x,text))return;
    fprintf(stderr,"Missing: %s\n",text);assert(!"Expected terminal cells missing");
}
static void capture(const char *directory,const char *name){
    char path[1024];FILE *f;unsigned n;if(!directory)return;
    assert(snprintf(path,sizeof path,"%s/%s.svg",directory,name)>0);
    n=ps_replay_svg(svg);assert(n&&n<sizeof svg);f=fopen(path,"wb");
    assert(f&&fwrite(svg,1,n,f)==n);assert(!fclose(f));
}
#if PS_MOUSE && PS_DIALOG_STYLES
static void click(ps_session *s,unsigned x,unsigned y){
    char bytes[80];int n=snprintf(bytes,sizeof bytes,"\033[<0;%u;%uM\033[<0;%u;%um",x+1,y+1,x+1,y+1);
    assert(n>0&&(unsigned)n<sizeof bytes);ps_feed(s,bytes,(unsigned)n);draw(s);
}
#endif
int main(int argc,char **argv){
    static const ps_control controls[]={
        {0,"Lighting","Output",PS_CONTROL_TOGGLE,0,1,1,0,0},
        {1,"Lighting","Brightness",PS_CONTROL_LEVEL,0,100,1,0,0},
        {2,"Connection","Hostname",PS_CONTROL_TEXT,0,0,0,0,0},
        {3,"Actions","Provision systems",PS_CONTROL_ACTION,0,0,0,0,0}};
    static const ps_control_page pages[]={{"Device",controls,PS_COUNT_OF(controls)}};
    static const char *const names[]={"frame-rounded","frame-single","frame-mixed","frame-heavy","frame-double"};
    ps_panel panel={"Device workspace","Preview provider",pages,PS_COUNT_OF(pages),read_value,write_value,0};
    ps_config config={sink,0,COLUMNS,ROWS,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE};
    void *memory=calloc(1,ps_sizeof_session());ps_session *s;unsigned style,part,y,x,label_y,label_x,top_y,top_x;
    const char *directory=argc>1?argv[1]:0;
    assert(memory);s=ps_init(memory,ps_sizeof_session(),&config);assert(s);
    ps_replay_reset();assert(ps_panel_attach(s,&panel)&&ps_panel_open(s,0));draw(s);
    locate("Hostname",&label_y,&label_x);locate(ps_border_glyph(ps_get_border_style(s),1,PS_BORDER_TOP_LEFT),&top_y,&top_x);
    assert(!ps_set_border_style(s,PS_BORDER_STYLE_COUNT));
    for(style=0;style<PS_BORDER_STYLE_COUNT;++style){
        if(!ps_border_style_valid(style)){assert(!ps_set_border_style(s,style));continue;}
        for(part=0;part<PS_BORDER_PART_COUNT;++part){uint32_t cp;const char *glyph=ps_border_glyph(style,1,part);
            assert(ps_rich_decode(glyph,strlen(glyph),&cp)==strlen(glyph));assert(ps_rich_cell_width(cp)==1);}
        assert(ps_set_border_style(s,style));draw(s);
        assert(text_at(label_y,label_x,"Hostname"));
        assert(text_at(top_y,top_x,ps_border_glyph(style,1,PS_BORDER_TOP_LEFT)));
        locate(ps_border_glyph(style,1,PS_BORDER_BOTTOM_RIGHT),&y,&x);assert(x==COLUMNS-2);
        capture(directory,names[style]);
        /* Re-styling an active editor cannot shift its origin or lose its draft. */
        assert(ps_panel_select(s,1));key(s,13);key(s,21);key(s,'7');key(s,'2');
        y=ps_replay_row();x=ps_replay_column();assert(ps_set_border_style(s,PS_BORDER_ROUNDED));draw(s);
        assert(ps_replay_row()==y&&ps_replay_column()==x);key(s,13);assert(level==72);
        level=64;assert(ps_panel_select(s,0));draw(s);
    }
    assert(ps_panel_select(s,3));key(s,13);capture(directory,"frame-dialog-before");key(s,27);
    assert(!ps_set_dialog_style(s,~0u));
#if PS_DIALOG_STYLES
    assert(ps_set_dialog_style(s,PS_DIALOG_TITLE_BAR|PS_DIALOG_FRAMED_BUTTONS));
    assert(ps_get_dialog_style(s)==(PS_DIALOG_TITLE_BAR|PS_DIALOG_FRAMED_BUTTONS));
    if(ps_border_style_valid(PS_BORDER_HEAVY_FRAME))assert(ps_set_border_style(s,PS_BORDER_HEAVY_FRAME));
    {unsigned before=writes,expected;key(s,13);capture(directory,"frame-dialog-after");
        locate("Apply",&y,&x);assert(text_at(y-1,x-3,ps_border_glyph(PS_BORDER_SINGLE,1,PS_BORDER_TOP_LEFT)));
        key(s,13);assert(writes==before); /* initial Cancel */
        key(s,13);key(s,9);key(s,13);assert(writes==before+1);
#if PS_MOUSE
        key(s,13);locate("Apply",&y,&x);click(s,x+9,y);assert(writes==before+1); /* between buttons */
        click(s,x-3,y-1);assert(writes==before+2); /* button border is clickable */
        key(s,13);locate("Cancel",&y,&x);click(s,x,y);assert(writes==before+2);
        key(s,13);locate("×",&y,&x);click(s,x,y);assert(writes==before+2);
#endif
        expected=writes;ps_resize(s,42,14);key(s,13);locate("Cancel",&y,&x);assert(y<14&&x<42);
        key(s,27);assert(writes==expected);
    }
#else
    assert(!ps_set_dialog_style(s,PS_DIALOG_TITLE_BAR));
#endif
    ps_close(s);config.flags=PS_INTERACTIVE;s=ps_init(memory,ps_sizeof_session(),&config);assert(s);
    ps_replay_reset();assert(ps_panel_attach(s,&panel)&&ps_panel_open(s,0));
    if(ps_border_style_valid(PS_BORDER_HEAVY_FRAME))assert(ps_set_border_style(s,PS_BORDER_HEAVY_FRAME));
    assert(ps_set_dialog_style(s,PS_DIALOG_STYLES?(PS_DIALOG_TITLE_BAR|PS_DIALOG_FRAMED_BUTTONS):0));
    assert(ps_panel_select(s,3));key(s,13);
    for(y=0;y<ROWS;++y)for(x=0;x<COLUMNS;++x)assert((unsigned char)ps_replay_cell(y,x)[0]<128);
    capture(directory,"frame-ascii");ps_close(s);free(memory);
    puts("Frames: one-cell glyphs, stable table/edit coordinates, shared dialog hit targets and ASCII fallback passed");return 0;
}
