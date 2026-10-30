#include "portable_meter.h"
#include <cassert>
#include <string>
#include <cstdint>
static void output(void *ctx,const unsigned char *p,size_t n){static_cast<std::string *>(ctx)->append((const char *)p,n);}
static int reject(void *,uint32_t){return 0;}
int main(){
    ps_meter m={0,UINT32_MAX,UINT32_MAX/2,0,17,1,1};
    assert(ps_meter_fraction(UINT32_MAX-1,UINT32_MAX,100)==99);
    for(unsigned span=1;span<400;++span)for(unsigned v=0;v<=span;++v)
        assert(ps_meter_fraction(v,span,100)==v*100/span);
    assert(ps_meter_adjust(&m,PS_METER_INCREASE,0,0)&&m.requested==17);
    assert(!ps_meter_adjust(&m,PS_METER_INCREASE,reject,0)&&m.requested==17);
    assert(ps_meter_position(&m,239,240,0,0)&&m.requested==UINT32_MAX);
    assert(!ps_meter_adjust(&m,PS_METER_INCREASE,0,0));
    assert(m.observed==UINT32_MAX/2);
    m.adjustable=0;assert(!ps_meter_position(&m,0,240,0,0));m.adjustable=1;
    ps_meter_style s={48,PS_METER_LINE,PS_METER_TEXT_ABOVE,1,0,0,"Download","Observed / target"};
    for(unsigned style=0;style<2;++style)for(unsigned text=0;text<3;++text)
    for(unsigned panel=0;panel<2;++panel)for(unsigned w=8;w<=240;++w){
        s.style=style;s.text=text;s.panel=panel;s.width=w;std::string out;
        unsigned rows=ps_meter_render(&m,&s,output,&out),lines=0;size_t pos=0,end;
        while((end=out.find("\r\n",pos))!=std::string::npos){assert(end-pos==w);pos=end+2;++lines;}
        assert(rows==lines&&out.find('\033')==std::string::npos);
    }
    s.width=48;s.unicode=1;s.color=1;s.style=PS_METER_BLOCKS;s.text=PS_METER_TEXT_INSIDE;
    std::string out;ps_meter_render(&m,&s,output,&out);assert(out.find("█")!=std::string::npos);
    m.maximum=0;out.clear();assert(!ps_meter_render(&m,&s,output,&out)&&out.empty());
}
