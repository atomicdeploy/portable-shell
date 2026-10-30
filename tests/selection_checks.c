#define PS_EDITOR_PROMPT_STYLE "\033[1;93m"
#include "portable_editor.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static char output[2048];static size_t used;
static void write_bytes(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;assert(n<sizeof output-used);memcpy(output+used,p,n);used+=n;output[used]=0;
}
static void feed(ps_editor *s,const char *text){while(*text)ps_editor_feed(s,(unsigned char)*text++,write_bytes,0);}
static const char *candidate(void *ctx,size_t index){(void)ctx;return index==0?"status":0;}
int main(void){
    ps_editor editor;char actual[80],expected[80];
    ps_editor_init(&editor);editor.echo=1;editor.prompt[0]=0;
    ps_editor_prompt(&editor,write_bytes,0);assert(strstr(output,"\033[1;93m> \033[0m"));
    feed(&editor,"ac\033[Db");assert(!strcmp(editor.command,"abc")&&editor.cursor==2);
    feed(&editor,"\177");assert(!strcmp(editor.command,"ac")&&editor.cursor==1);
    feed(&editor,"\014");assert(!strcmp(editor.command,"ac")&&editor.cursor==1);
    assert(ps_editor_remember(&editor,1)==(PS_EDITOR_HISTORY!=0));
    ps_editor_history(&editor,1,write_bytes,0);
#if !PS_EDITOR_HISTORY
    assert(!strcmp(editor.command,"ac")&&strchr(output,'\a'));
#endif
    ps_editor_clear(&editor);feed(&editor,"sta");
    ps_editor_suggest(&editor,0,1,candidate,0,write_bytes,0);
    assert(!strcmp(editor.suggestion,"tus"));
    ps_editor_complete(&editor,0,1,candidate,0,write_bytes,0);
    assert(!strcmp(editor.command,"status "));
    snprintf(expected,sizeof expected,"%d",INT_MIN);
    assert(ps_terminal_format(actual,sizeof actual,"%d",INT_MIN)==strlen(expected));assert(!strcmp(actual,expected));
    snprintf(expected,sizeof expected,"%d",INT_MAX);ps_terminal_format(actual,sizeof actual,"%d",INT_MAX);assert(!strcmp(actual,expected));
    assert(ps_terminal_format(actual,3,"%d",-123)==4&&!strcmp(actual,"-1"));
    used=0;ps_terminal_number_pattern(write_bytes,0,"a%/%%",-42);assert(!strcmp(output,"a-42/-42-42"));
    printf("editor=%zu history=%d undo=%d\n",sizeof editor,PS_EDITOR_HISTORY,PS_EDITOR_UNDO_DEPTH);
    return 0;
}
