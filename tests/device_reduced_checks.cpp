#include "portable_device_view.hpp"
#include <cassert>
#include <string>
using V=PortableShell::DeviceView;
int main(){
    V v;V::Snapshot s;V::Action a;V::Row rows[48];
    for(unsigned i=0;i<48;++i){rows[i]={"Setting","setting","50",true};rows[i].category=i<24?"Outputs":"Settings";}
    rows[0].kind=V::Level;rows[0].minimum=0;rows[0].maximum=100;
    rows[1].kind=V::Toggle;rows[1].value="on";
    const char *choices[]={"slow","fast"};rows[2].kind=V::Choice;rows[2].value="slow";rows[2].choices=choices;rows[2].choiceCount=2;
    s.rows=rows;s.rowCount=48;s.columns=160;s.lines=40;s.unicode=true;s.terminalSettings=true;
    v.begin();std::string out;
    auto render=[&](){out.clear();v.render(s,[&](const uint8_t *p,size_t n){out.append((const char*)p,n);},true);};
    auto feed=[&](const char *p){bool sent=false;while(*p)sent=v.feed(uint8_t(*p++),s,a)||sent;return sent;};
    render();
    assert(out.find("Value")!=std::string::npos);
#if PS_DEVICE_VIEW_SECTIONS
    assert(out.find("OUTPUTS")!=std::string::npos);
#endif
    assert(out.find("\x1b[7m")!=std::string::npos);
    assert(out.find("\x1b[7;159H")!=std::string::npos);
    unsigned labels=0;for(size_t p=0;(p=out.find("Setting",p))!=std::string::npos;++p)++labels;
    assert(labels>8);
    for(size_t p=0;(p=out.find("\x1b[",p))!=std::string::npos;++p){
        size_t end=p+2;while(end<out.size()&&(out[end]<'@'||out[end]>'~'))++end;
        if(end<out.size()&&out[end]=='m'){
            std::string sgr=out.substr(p+2,end-p-2);
            assert(sgr=="0"||sgr=="1"||sgr=="7"||sgr=="1;7");
        }
    }
    assert(!feed("\x1bOQ")&&!v.editing());
    assert(!feed("\x1b[12~")&&!v.editing());
    assert(!feed("\x1b[<2;70;9M")&&!v.editing());
    feed("\x1b[H");if(v.sectionSelected())feed("\x1b[B");
    feed("\r\x15" "101");assert(!feed("\r")&&v.editing());
    feed("\x15" "75");assert(feed("\r")&&std::string(a.value)=="75");
    feed("\x1b[B\r\x15" "maybe");assert(!feed("\r")&&v.editing());
    feed("\x15" "off");assert(feed("\r")&&std::string(a.value)=="off");
    feed("\x1b[B\r\x15" "other");assert(!feed("\r")&&v.editing());
    feed("\x15" "fast");assert(feed("\r")&&std::string(a.value)=="fast");
    #if PS_DEVICE_VIEW_SECTIONS
    feed("\x1b[H");assert(v.sectionSelected());feed(" ");render();
    assert(out.find("Section collapsed")!=std::string::npos);
    feed(" ");
#endif
    v.setTabLayout(V::VerticalTabs);v.setTabsInsideTable(true);render();
    assert(out.find("Pages")!=std::string::npos);
    v.setSelectionMarker("* ");feed("\x1b[H");if(v.sectionSelected())feed("\x1b[B");render();
    assert(out.find("* Setting")!=std::string::npos);
    return 0;
}
