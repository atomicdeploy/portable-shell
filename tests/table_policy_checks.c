#include "portable_shell.h"
#include "portable_terminal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static union {void *align;unsigned char bytes[65536];} memory;
static char output[65536],plain[65536];static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;}
static int read_value(void *ctx,unsigned id,ps_control_value *v){(void)ctx;(void)id;v->flags=PS_CONTROL_VALID;v->number=1;return 1;}
static void check(unsigned style,const char *group,const char *marker,unsigned unicode,int expected){
#ifdef PS_ASCII_ONLY
    unicode=0;if(!marker)expected=1;
#endif
    ps_config cfg={sink,0,100,24,PS_INTERACTIVE|(unicode?PS_UNICODE:0)};
    ps_control control={1,group,"First",PS_CONTROL_TOGGLE,0,1,1,0,0};
    ps_control_page page={"Page",&control,1};
    ps_panel panel={"Panel","",&page,1,read_value,0,0};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(s);
    ps_set_table_style(s,PS_TABLE_DEFAULT|style);ps_set_selection_marker(s,marker);
    assert(ps_panel_attach(s,&panel));used=0;
    assert(ps_panel_open(s,0));ps_tick(s,100);
    {unsigned i=0,n=0;while(i<used){if(output[i]==27)i+=(unsigned)ps_escape_span(output+i,used-i);else plain[n++]=output[i++];}plain[n]=0;}
    if((strstr(plain,unicode?"▸ First":"> First")!=0)!=expected)fprintf(stderr,"style=%u group=%s marker=%s unicode=%u output=%s\n",style,group,marker?marker:"auto",unicode,plain);
    assert((strstr(plain,unicode?"▸ First":"> First")!=0)==expected);
}
int main(void){
    check(0,"",0,1,1);
#if PS_CONTROL_SECTION_ROWS
    check(0,"Group",0,1,0);
#endif
    check(PS_TABLE_STATIC_SECTIONS,"Group",0,1,1);
    check(PS_TABLE_SECTION_PLUS_MINUS,"Group",0,1,1);
    check(PS_TABLE_SECTION_NO_ICONS,"Group",0,1,1);
    check(PS_TABLE_CATEGORY_COLUMN,"Group",0,1,1);
    check(PS_TABLE_STATIC_SECTIONS,"Group","",1,0);
    check(0,"","> ",0,1);
    check(0,"Group",0,0,1);
    return 0;
}



