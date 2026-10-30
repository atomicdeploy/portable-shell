#include "portable_rich.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char captured[131072],svg[1200000];static size_t used;
static void output(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;assert(used+n<sizeof captured);memcpy(captured+used,p,n);used+=n;captured[used]=0;ps_replay_feed(p,(unsigned)n);
}
static void reset(void){used=0;captured[0]=0;ps_replay_reset();}
static void at(ps_rich*r,unsigned row,unsigned col){char b[32];ps_terminal_format(b,sizeof b,"\033[%u;%uH",row,col);ps_rich_literal(r,b);}
static void label(ps_rich*r,unsigned row,const char*s){at(r,row,3);ps_rich_color(r,0x56b6c2,0);ps_rich_literal(r,s);}
int main(int argc,char **argv){
    ps_rich r={output,0,PS_COLOR_RGB,0,0};uint32_t cp;unsigned row,i,mode;
    assert(ps_rich_decode("\xf0\x9f\x9a\x80",4,&cp)==4&&cp==0x1f680);
    assert(ps_rich_decode("\xed\xa0\x80",3,&cp)==1&&cp==0xfffd);
    assert(ps_rich_decode("\xe4",1,&cp)==1&&cp==0xfffd);
    assert(ps_rich_measure(&r,"A中e\xcc\x81",7)==4);
    reset();ps_rich_text(&r,"中A",4,2);assert(!strcmp(captured,"中"));
    reset();ps_rich_text(&r,"\033[2J",4,9);assert(!strchr(captured,27));
    assert(ps_rich_nearest(0xff0000,16)==9);
    reset();r.color=PS_COLOR_256;ps_rich_color(&r,0xff0000,0);assert(strstr(captured,"38;5;9m"));
    reset();r.color=PS_COLOR_NONE;ps_rich_color(&r,0xff0000,0);ps_rich_attributes(&r,127);assert(!used);
    ps_rich_field(&r,"中",6,PS_ALIGN_RIGHT);assert(!strcmp(captured,"    中"));
    reset();assert(ps_rich_wrap(&r,"one two three",7,PS_ALIGN_LEFT)==2);assert(!strcmp(captured,"one two\r\nthree\r\n"));
    reset();ps_rich_wrap(&r,"a b ccccc",5,PS_ALIGN_FULL);assert(!strcmp(captured,"a   b\r\nccccc\r\n"));
#if PS_RICH_MARKUP
    reset();assert(!ps_rich_markup(&r,"[bold]safe[/] [[literal"));assert(!strcmp(captured,"safe [literal"));
    reset();assert(ps_rich_markup(&r,"[/]")==1);
#endif
#if PS_RICH_DOCUMENT
    reset();assert(ps_rich_document(&r,"# Heading\n- item\n> quote",30)==3);assert(!strchr(captured,27));
#endif
#if PS_RICH_SYNTAX
    reset();ps_rich_code(&r,"{\"value\": 42}\n");assert(!strcmp(captured,"{\"value\": 42}\r\n"));
#endif
    reset();r.color=PS_COLOR_RGB;ps_rich_attributes(&r,127);
    assert(strstr(captured,"\033[1m")&&strstr(captured,"\033[3m")&&strstr(captured,"\033[5m")&&strstr(captured,"\033[9m"));
    if(argc==1)return 0;
    if(argc!=2)return 2;
    reset();r.color=PS_COLOR_RGB;
    ps_rich_literal(&r,"\033[48;2;40;44;52m\033[2J");label(&r,1,"PORTABLE TEXT  /  capability-driven rendering");
    for(mode=PS_COLOR_16;mode<=PS_COLOR_RGB;++mode){
        r.color=PS_COLOR_RGB;label(&r,3+(mode-1)*2,mode==PS_COLOR_16?"16 colors":mode==PS_COLOR_256?"256 colors":"Truecolor");
        at(&r,3+(mode-1)*2,20);r.color=mode;
        for(i=0;i<64;++i){unsigned phase=i*6/64,t=(i*6%64)*255/64;uint32_t rgb=phase==0?0xff0000|(t<<8):phase==1?((255-t)<<16)|0xff00:phase==2?0xff00|t:phase==3?((255-t)<<8)|255:phase==4?(t<<16)|255:0xff0000|(255-t);ps_rich_color(&r,rgb,1);ps_rich_raw(&r," ",1);}
        ps_rich_reset(&r);
    }
    r.color=PS_COLOR_RGB;label(&r,9,"ALIGNMENT");row=10;
    for(i=0;i<4;++i){
        at(&r,row+i*2,3);ps_rich_color(&r,0xabb2bf,0);ps_rich_field(&r,i==0?"Left":i==1?"Center":i==2?"Right":"Full justification",20,0);
        at(&r,row+i*2,1);ps_rich_wrap_indented(&r,i==3?"Words distribute across the available width with a final short line.":"Measured terminal text",48,i,22);
        at(&r,row+i*2,3);ps_rich_field(&r,i==0?"Left":i==1?"Center":i==2?"Right":"Full justification",20,0);
    }
    label(&r,19,"SAFE UTF-8");at(&r,20,3);ps_rich_color(&r,0x98c379,0);ps_rich_literal(&r,"Preserved text: ");ps_rich_text(&r,"中文  日本語  한국어  café  🚀",ps_text_length("中文  日本語  한국어  café  🚀"),70);
#if PS_RICH_MARKUP
    label(&r,22,"MARKUP");at(&r,23,3);ps_rich_markup(&r,"[#98c379]Ready[/]  [#e5c07b]Pending[/]  [#e06c75]Failed[/]  [[literal bracket");
#endif
#if PS_RICH_SYNTAX
    label(&r,25,"LEXICAL HIGHLIGHTING");at(&r,26,3);ps_rich_code(&r,"{\"active\": true, \"progress\": 73, \"message\": \"Connected\"}");
#endif
#if PS_RICH_DOCUMENT
    label(&r,28,"DOCUMENT BLOCKS");at(&r,29,1);ps_rich_document(&r,"# Deployment\n- Stream events through your own transport\n> Caller-owned storage; no allocation",80);
#endif
    label(&r,33,"STYLES");at(&r,34,3);
#if PS_RICH_MARKUP
    ps_rich_markup(&r,"[bold]Bold[/]  [dim]Dim[/]  [italic]Italic[/]  [underline]Underline[/]  [strike]Strike[/]  [reverse]Reverse[/]");
#endif
    {FILE*f=fopen(argv[1],"wb");unsigned n=ps_replay_svg(svg);assert(f);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));}
    return 0;
}
