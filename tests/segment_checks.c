#include "portable_segments.h"
#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char output[32768],svg[400000];static unsigned used;
static void sink(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=(unsigned)n;output[used]=0;
    ps_replay_feed(p,(unsigned)n);
}
static void shell_sink(void *ctx,const ps_byte *p,unsigned n){sink(ctx,p,n);}
static union {void *align;unsigned char bytes[PS_DEFAULT_SESSION_STORAGE];} memory;
static void draw(const char *text,ps_segment_style *style){
    unsigned short cells[32];int n=ps_segment_parse(text,cells,32);
    assert(n>0);assert(ps_segment_render(cells,(unsigned)n,style,95,sink,0)>0);
}
int main(int argc,char **argv){
    static const unsigned expected[]={63,6,91,79,102,109,125,7,127,111};
    unsigned short cells[32];unsigned i;ps_segment_style s=ps_segment_default_style();
    ps_config cfg={shell_sink,0,96,24,0};ps_session *session;
    for(i=0;i<10;++i)assert(ps_segment_mask('0'+i)==expected[i]);
    assert(ps_segment_parse("-12.5: ",cells,32)==6);
    assert(cells[2]==(91|PS_SEG_DP)&&cells[4]==PS_SEG_COLON);
    assert(ps_segment_parse("?",cells,32)==-1);
    assert(ps_segment_parse("123",cells,2)==-1);
    assert(ps_segment_parse("..",cells,32)==2);
    assert(ps_segment_parse("88",cells,32)==2);
    assert(ps_segment_width(cells,2,&s)==14);
    used=0;assert(!ps_segment_render(cells,2,&s,13,sink,0)&&!used);
    assert(ps_segment_render(cells,2,&s,14,sink,0)==7);
    assert(strstr(output,"\x1b[38;5;208m")&&strstr(output,"━"));
    used=0;s.color=s.unicode=0;assert(ps_segment_render(cells,2,&s,14,sink,0)==7);
    assert(!strchr(output,27)&&strchr(output,'-')&&strchr(output,'|'));
    s.horizontal=0;used=0;assert(!ps_segment_render(cells,2,&s,95,sink,0)&&!used);
    session=ps_init(memory.bytes,sizeof memory.bytes,&cfg);used=0;
    assert(ps_execute(session,"segments 12.5 --ascii")==0);assert(!strchr(output,27));
    assert(ps_execute(session,"segments 12 --invalid")!=0);
    if(argc==1)return 0;
    if(argc!=2)return 2;
    used=0;ps_replay_reset();s=ps_segment_default_style();
    draw("12:48.6",&s);ps_segment_emit(sink,0,"\r\n");
    s.foreground=203;s.horizontal=2;s.vertical=1;draw("-23.5",&s);
    ps_segment_emit(sink,0,"\r\n");s.color=s.unicode=0;draw("0123456789",&s);
    {
        unsigned n=ps_replay_svg(svg);FILE *f=fopen(argv[1],"wb");int bad;
        if(!f)return 3;
        bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
    }
}
