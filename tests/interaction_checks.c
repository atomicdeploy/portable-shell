#include "portable_screen.h"
#include "portable_editor.h"
#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char output[262144],svg[1200000];static unsigned used;
static void sink(void *ctx,const unsigned char*p,size_t n){(void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=(unsigned)n;output[used]=0;ps_replay_feed(p,(unsigned)n);}
#if PS_UNDO_DEPTH
static void shell_sink(void *ctx,const ps_byte*p,unsigned n){sink(ctx,p,n);}
#endif
static ps_cell cells[96*24],presented[96*24],pad_cells[40*40],pad_presented[40*40];
#if PS_UNDO_DEPTH
static union {void *align;unsigned char bytes[48000];} memory;
#endif
static unsigned hex_saved,hex_fail;
static int save_hex(void *ctx,const ps_byte *p,unsigned n){(void)ctx;if(hex_fail)return 0;assert(n==4&&p[0]==0xAF);++hex_saved;return 1;}
int main(int argc,char **argv){
    ps_screen screen,pad;ps_window whole,view,source;ps_rich rich={sink,0,PS_COLOR_RGB,0,0};
    ps_cell style=ps_cell_make(0xabb2bf,0x181a1f,0),heading=ps_cell_make(0x70d3eb,0x181a1f,PS_TEXT_BOLD);
    ps_editor editor;unsigned i,cursor=0;char snapshots[3][16],line[16];unsigned positions[3];ps_undo undo;
    ps_undo_bind(&undo,&snapshots[0][0],positions,16,3);
    assert(ps_undo_record(&undo,"",0)&&ps_undo_record(&undo,"a",1)&&ps_undo_record(&undo,"ab",2));
    assert(ps_undo_step(&undo,0,line,&cursor)&&!strcmp(line,"a")&&cursor==1);
    assert(ps_undo_step(&undo,1,line,&cursor)&&!strcmp(line,"ab"));
    assert(ps_undo_step(&undo,0,line,&cursor));ps_undo_record(&undo,"z",1);assert(!ps_undo_step(&undo,1,line,&cursor));
    ps_undo_clear(&undo);assert(!snapshots[0][0]&&!undo.count);
    ps_editor_init(&editor);
    {const char *paste="\033[200~echo one\nsecond\003\033[201~";for(i=0;paste[i];++i)assert(ps_editor_feed(&editor,(unsigned char)paste[i],sink,0)==PS_EDIT_NONE);}
    assert(!strcmp(editor.command,"echo one second"));
#if PS_EDITOR_UNDO_DEPTH
    ps_editor_feed(&editor,31,sink,0);assert(!editor.command[0]);
    ps_editor_feed(&editor,27,sink,0);ps_editor_feed(&editor,'_',sink,0);assert(!strcmp(editor.command,"echo one second"));
#endif
    assert(ps_editor_feed(&editor,13,sink,0)==PS_EDIT_SUBMIT);
    ps_editor_clear(&editor);ps_editor_feed(&editor,'a',sink,0);ps_editor_feed(&editor,'b',sink,0);
#if PS_EDITOR_UNDO_DEPTH
    ps_editor_feed(&editor,31,sink,0);assert(!strcmp(editor.command,"a"));
    ps_editor_feed(&editor,27,sink,0);ps_editor_feed(&editor,'_',sink,0);assert(!strcmp(editor.command,"ab"));
    {ps_editor clone=editor;ps_editor_clear(&clone);assert(!strcmp(editor.command,"ab"));}
#endif
#if PS_UNDO_DEPTH
    {ps_config config={shell_sink,0,96,24,PS_INTERACTIVE};ps_session*s=ps_init(memory.bytes,sizeof memory.bytes,&config);assert(s);
        ps_feed(s,(const ps_byte*)"echo ab",7);ps_key(s,31);ps_key(s,13);
        assert(!strcmp(ps_history_get(s,0),"echo a"));
        ps_feed(s,(const ps_byte*)"echo xy",7);ps_key(s,31);ps_key(s,PS_KEY_REDO);ps_key(s,13);
        assert(!strcmp(ps_history_get(s,1),"echo xy"));
        {const char *paste="\033[200~echo pasted\033[201~";ps_feed(s,(const ps_byte*)paste,(unsigned)strlen(paste));}
        ps_key(s,31);ps_key(s,13);assert(ps_history_count(s)==2);
        ps_feed(s,"!!\r",3);assert(ps_history_count(s)==2);
        ps_feed(s,"!1\r",3);assert(!strcmp(ps_history_get(s,2),"echo a"));
        ps_feed(s,"draft",5);ps_feed(s,"\033[18~",5);
        assert(ps_terminal_state(s)==PS_TERMINAL_ALT_SCREEN);
        ps_resize(s,40,10);ps_key(s,27);assert(!ps_terminal_state(s));
        ps_key(s,13);assert(!strcmp(ps_history_get(s,3),"draft"));
        ps_key(s,PS_KEY_F7);ps_key(s,PS_KEY_HOME);ps_key(s,9);
        ps_feed(s," edited\r",8);assert(!strcmp(ps_history_get(s,4),"echo a edited"));
        ps_key(s,PS_KEY_F7);ps_key(s,PS_KEY_HOME);ps_key(s,13);
        assert(!strcmp(ps_history_get(s,5),"echo a"));
        ps_feed(s,"!999\r",5);assert(ps_history_count(s)==6&&ps_status(s)==2);
        ps_key(s,3);ps_key(s,PS_KEY_F7);ps_close(s);assert(!ps_terminal_state(s));
    }
#endif
#if PS_UNDO_DEPTH && PS_HEX_EDITOR
    {ps_config config={shell_sink,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_UNICODE};
     ps_byte original[]={0,127,128,255},draft[4];ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&config);
     assert(ps_hex_editor_open(s,"Binary",original,sizeof original,draft,sizeof draft,save_hex,0));
     assert(ps_terminal_state(s)==PS_TERMINAL_ALT_SCREEN);
     ps_feed(s,"af",2);assert(draft[0]==0xAF&&original[0]==0);
     ps_key(s,9);ps_key(s,'Z');assert(draft[1]=='Z');
     ps_resize(s,24,8);hex_fail=1;ps_key(s,19);assert(!hex_saved&&ps_hex_editor_active(s));
     ps_key(s,27);ps_key(s,27);assert(ps_hex_editor_active(s));
     hex_fail=0;ps_key(s,19);assert(hex_saved==1);ps_key(s,27);assert(!ps_hex_editor_active(s));
     assert(ps_hex_editor_open(s,"Discard",original,4,draft,4,save_hex,0));
     ps_key(s,'1');ps_key(s,27);ps_key(s,'d');assert(!ps_hex_editor_active(s)&&hex_saved==1&&original[0]==0);
     assert(!ps_hex_editor_open(s,"Too small",original,4,draft,3,save_hex,0));
     assert(ps_hex_editor_open(s,"Empty",0,0,draft,4,save_hex,0));ps_key(s,PS_KEY_END);ps_key(s,'F');ps_key(s,27);
    }
#endif
    assert(ps_screen_init(&screen,cells,presented,96,24));whole=ps_screen_window(&screen,0,0,96,24);
    ps_window_fill(&whole,style);used=0;ps_replay_reset();assert(ps_screen_present(&screen,&rich)==96*24);
    used=0;assert(!ps_screen_present(&screen,&rich)&&!used);
    assert(ps_window_put(&whole,4,3,"中",style));assert(ps_window_put(&whole,5,3,"X",style));
    assert(cells[3*96+4].glyph[0]==' '&&cells[3*96+5].glyph[0]=='X');
    view=ps_screen_window(&screen,94,23,40,40);assert(view.width==2&&view.height==1);
    assert(!ps_window_put(&view,1,0,"中",style));
    assert(ps_screen_present(&screen,&rich)==1); /* Only X differs after wide overwrite. */
    assert(ps_screen_init(&pad,pad_cells,pad_presented,40,40));
    source=ps_screen_window(&pad,0,0,40,40);ps_window_fill(&source,style);
    for(i=0;i<40;++i){char b[64];ps_terminal_format(b,sizeof b,"Event %02u  bounded event history",i);ps_window_text(&source,0,i,b,style);}
    view=ps_screen_window(&screen,51,5,39,13);source=ps_screen_window(&pad,0,12,39,13);assert(ps_window_blit(&view,&source));
    ps_window_scroll(&view,2,style);assert(!memcmp(cells[5*96+51].glyph,"E",1));
    assert(cells[5*96+57].glyph[0]=='1'&&cells[5*96+58].glyph[0]=='4');
    if(argc==1)return 0;
    if(argc!=2)return 2;
    ps_window_fill(&whole,style);
    view=ps_screen_window(&screen,2,1,92,3);ps_window_border(&view,heading,1);ps_window_text(&view,2,1,"RETAINED SCREEN  |  clipped views + changed-cell updates",heading);
    view=ps_screen_window(&screen,2,4,44,17);ps_window_border(&view,style,1);
    ps_window_text(&view,2,1,"Line editing",heading);ps_window_text(&view,2,3,"Ctrl-_   Undo",style);ps_window_text(&view,2,4,"Alt-_    Redo",style);
    ps_window_text(&view,2,6,"Bracketed paste stays input.",style);ps_window_text(&view,2,8,"Shared bounded undo journal",style);
    ps_window_text(&view,2,10,"No allocator or OS calls",style);
    view=ps_screen_window(&screen,48,4,46,17);ps_window_border(&view,style,1);ps_window_text(&view,2,1,"Scrollable pad  |  rows 12-23",heading);
    view=ps_screen_window(&screen,50,7,40,12);source=ps_screen_window(&pad,0,12,40,12);ps_window_blit(&view,&source);
    ps_window_text(&whole,3,22,"Unchanged frame: 0 bytes  |  host owns timing, input and terminal size",heading);
    ps_replay_reset();used=0;ps_screen_invalidate(&screen);ps_screen_present(&screen,&rich);
    {FILE*f=fopen(argv[1],"wb");unsigned n=ps_replay_svg(svg);assert(f);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));}
    return 0;
}
