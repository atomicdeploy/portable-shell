#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
unsigned ps_replay_row(void);
unsigned ps_replay_column(void);
const char *ps_replay_cell(unsigned,unsigned);
unsigned ps_replay_foreground(unsigned,unsigned);
enum { COLUMNS=64, ROWS=16, OUTPUT_BYTES=262144, SVG_BYTES=1000000 };
static char output[OUTPUT_BYTES],svg[SVG_BYTES],received[PS_LINE];
static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);
}
static void clear_output(void){used=0;output[0]=0;}
static void keys(ps_session *s,const char *p){while(*p)ps_key(s,(unsigned char)*p++);}
static unsigned occurrences(const char *needle){unsigned n=0;const char *p=output;while((p=strstr(p,needle))){++n;p+=strlen(needle);}return n;}
static int at(unsigned row,unsigned col,const char *text){
    while(*text&&col<COLUMNS){const char *p=ps_replay_cell(row,col++);size_t n=strlen(p);if(!n||strncmp(p,text,n))return 0;text+=n;}return !*text;
}
static void locate(const char *text,unsigned *y,unsigned *x){
    for(*y=0;*y<ROWS;++*y)for(*x=0;*x<COLUMNS;++*x)if(at(*y,*x,text))return;
    assert(!"Text missing from emitted terminal screen");
}
static int capture(ps_session *s,int argc,const char *const *argv,void *ctx){
    (void)argc;(void)ctx;assert(argv[1]);strcpy(received,argv[1]);
    ps_message(s,PS_STYLE_SUCCESS,"acknowledged");return 23;
}
static int read_value(void *ctx,unsigned id,ps_control_value *v){
    (void)ctx;assert(id==0);memset(v,0,sizeof *v);v->flags=PS_CONTROL_VALID;strcpy(v->text,"draft");return 1;
}
static int write_value(void *ctx,unsigned id,const ps_control_value *v,ps_u32 rev){
    (void)ctx;(void)id;(void)rev;strcpy(received,v->text);return 1;
}
static void save(const char *path){FILE *f;unsigned n;if(!path)return;n=ps_replay_svg(svg);assert(n<sizeof svg);f=fopen(path,"wb");assert(f);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));}
int main(int argc,char **argv){
    ps_config c={sink,0,COLUMNS,ROWS,PS_INTERACTIVE|PS_UNICODE|PS_COLOR|PS_TRUECOLOR|PS_NO_BUILTINS};
    ps_prompt_part p[]={{"╭─ operator@workbench",PS_STYLE_SUCCESS},{"  ● Ready\n",PS_STYLE_MUTED},{"╰─❯ ",PS_STYLE_COMMAND}};
    const ps_report_row rows[]={
        {"Playback",0,0,PS_REPORT_SECTION},
        {"State",0,"● Playing",PS_REPORT_SUCCESS},
        {"Position",0,"01:24 / 04:12",0},
        {"Volume",0,"72%",PS_REPORT_NUMBER},
        {"Connection",0,0,PS_REPORT_SECTION|PS_REPORT_DIVIDER},
        {"Controller",0,"● Online",PS_REPORT_SUCCESS},
        {"Sync",0,"○ Waiting",PS_REPORT_WARNING}
    };
    ps_command cmd={"capture","<text>","",capture,0,0};
    ps_session *s;void *memory=calloc(1,ps_sizeof_session());unsigned y,x,row,col;
    assert(memory);s=ps_init(memory,ps_sizeof_session(),&c);assert(s);
    assert(!ps_register(s,&cmd));assert(!ps_set_prompt_parts(s,p,PS_COUNT_OF(p)));ps_replay_reset();ps_start(s);
    keys(s,"capture abcd");ps_key(s,PS_KEY_LEFT);ps_key(s,PS_KEY_LEFT);
    col=ps_replay_column();clear_output();ps_set_status(s,-7);
    assert(ps_message_block(s,PS_STYLE_SUCCESS,"First response\r\nSecond response\n",PS_LITERAL_LENGTH("First response\r\nSecond response\n")));
    assert(occurrences("operator@workbench")==1&&ps_status(s)==-7);
    assert(ps_replay_column()==col);locate("First response",&y,&x);assert(y==0);
    locate("Second response",&y,&x);assert(y==1);
    locate("capture abcd",&y,&x);assert(y==3);
    clear_output();ps_message(s,PS_STYLE_WARNING,"Single event");
    assert(occurrences("operator@workbench")==1&&ps_status(s)==-7&&ps_replay_column()==col);
    clear_output();ps_message(s,PS_STYLE_DEFAULT,"");
    assert(occurrences("operator@workbench")==1&&ps_status(s)==-7&&ps_replay_column()==col);
    /* Live header changes erase every previous row before replacing the parts. */
    clear_output();assert(!ps_set_prompt(s,"$ ",PS_STYLE_SUCCESS));assert(!strstr(output,"operator@workbench"));
    assert(at(ps_replay_row(),0,"$ capture abcd"));
    assert(!strcmp(ps_replay_cell(ps_replay_row()+1,0)," "));
    assert(!ps_set_prompt_parts(s,p,PS_COUNT_OF(p)));
    ps_resize(s,24,ROWS);col=ps_replay_column();clear_output();
    {const char bounded[]={'b','o','u','n','d','e','d','\n',27,'[','2','J','x',7};
        assert(ps_message_block(s,PS_STYLE_DEFAULT,bounded,sizeof bounded)==sizeof bounded);
        assert(!strstr(output,"\033[2J")&&!strchr(output,7));assert(ps_replay_column()==col);
    }
    assert(ps_status(s)==-7);ps_resize(s,COLUMNS,ROWS);
    ps_key(s,'X');ps_key(s,13);assert(!strcmp(received,"abXcd")&&ps_status(s)==23);
    ps_resize(s,24,ROWS);keys(s,"capture abcdefghijklmnopqrstuvwxyz");
    ps_key(s,PS_KEY_LEFT);ps_key(s,PS_KEY_LEFT);col=ps_replay_column();ps_set_status(s,0);clear_output();
    ps_message(s,PS_STYLE_SUCCESS,"An event that wraps safely within this narrow viewport.");
    assert(ps_replay_column()==col&&ps_status(s)==0&&occurrences("operator@workbench")==1);
    ps_key(s,'X');ps_key(s,13);assert(!strcmp(received,"abcdefghijklmnopqrstuvwxXyz"));
    /* Fresh viewport: aligned report + an unfinished draft is the gallery fixture. */
    s=ps_init(memory,ps_sizeof_session(),&c);assert(s);assert(!ps_register(s,&cmd));
    ps_replay_reset();assert(!ps_set_prompt_parts(s,p,PS_COUNT_OF(p)));ps_start(s);
    keys(s,"capture unfinished");ps_key(s,PS_KEY_LEFT);ps_key(s,PS_KEY_LEFT);col=ps_replay_column();
    ps_set_status(s,9);clear_output();
#if PS_REPORT
    assert(ps_message_report(s,rows,PS_COUNT_OF(rows),PS_REPORT_COLUMNS)==PS_COUNT_OF(rows));
    assert(occurrences("operator@workbench")==1&&ps_replay_column()==col&&ps_status(s)==9);
    {const ps_report_row invalid={"Unsafe",0,"\033[2J",0};clear_output();
        assert(!ps_message_report(s,&invalid,1,PS_REPORT_COLUMNS)&&!used&&ps_status(s)==9);}
    locate("01:24",&row,&col);locate("72%",&y,&x);assert(x==col);
#ifndef PS_ASCII_ONLY
    assert(ps_replay_foreground(row,col)!=ps_replay_foreground(y,x));
#else
    {unsigned i;for(i=0;i<used;++i)assert((unsigned char)output[i]<128);}
#endif
    if(argc>1)save(argv[1]);
#else
    (void)argc;(void)argv;(void)row;(void)save;
    assert(!ps_message_report(s,rows,PS_COUNT_OF(rows),PS_REPORT_COLUMNS)&&!used&&ps_status(s)==9);
#endif
    /* One concise notice, no report body, while table inline editing is active. */
    {const ps_control control={0,"","Label",PS_CONTROL_TEXT,0,0,0,0,0};
        const ps_control_page page={"Settings",&control,1};
        const ps_panel panel={"Event table","",&page,1,read_value,write_value,0};
        assert(ps_panel_attach(s,&panel));assert(ps_panel_open(s,0));assert(ps_panel_select(s,0));
        ps_key(s,13);ps_key(s,21);keys(s,"preserved");clear_output();
        ps_message_block(s,PS_STYLE_SUCCESS,"Saved state\nHidden detail",PS_LITERAL_LENGTH("Saved state\nHidden detail"));
        assert(!strstr(output,"Hidden detail"));locate("Saved state",&y,&x);
#if PS_REPORT
        clear_output();assert(!ps_message_report(s,rows,PS_COUNT_OF(rows),PS_REPORT_COLUMNS));
        locate("State: ",&y,&x);assert(!strstr(output,"Position")&&ps_status(s)==9);
#endif
        ps_key(s,13);assert(!strcmp(received,"preserved"));ps_key(s,27);
    }
#if PS_EDITOR
    assert(ps_editor_open(s,"Document","unsaved text",0,0));clear_output();
    ps_message(s,PS_STYLE_WARNING,"Connection changed\nHidden detail");
    assert(!strstr(output,"Hidden detail")&&!strcmp(ps_editor_text(s),"unsaved text"));locate("Connection changed",&y,&x);
#endif
    ps_close(s);free(memory);puts("Async blocks/reports preserve multiline prompt, input cursor, status and editor drafts");return 0;
}
