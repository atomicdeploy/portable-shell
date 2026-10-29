#include "portable_device_view.hpp"
#include <cassert>
#include <cstdio>
#include <string>
using V=PortableShell::DeviceView;
#if PS_DEVICE_VIEW_TABLE && PS_DEVICE_VIEW_WIDGETS && PS_DEVICE_VIEW_DIALOGS && PS_STYLE_THEMES
static void widths(const std::string &output,unsigned maximum){
    unsigned column=0;bool escape=false;
    for(unsigned char c:output){
        if(c==27){escape=true;continue;}
        if(escape){if(c>=0x40&&c<=0x7e&&c!='[')escape=false;continue;}
        if(c=='\r')column=0;else if(c=='\n')assert(column<=maximum);
        else if((c&0xc0)!=0x80){++column;assert(column<=maximum);}
    }
}
int main(){
    V v;V::Snapshot s;V::Action a;V::Row rows[64];
    const char *choices[]={"effects","static"};
    for(auto &r:rows)r={"Setting","setting","observed",true};
    rows[0]={"Output","output","true",true};rows[0].kind=V::Toggle;
    rows[1]={"Brightness","brightness","128",true};rows[1].kind=V::Level;rows[1].minimum=1;rows[1].maximum=255;rows[1].meter=true;
    rows[2]={"Mode","mode","effects",true};rows[2].kind=V::Choice;rows[2].choices=choices;rows[2].choiceCount=2;
    for(auto &r:rows)r.category="Lighting";
    s.rows=rows;s.rowCount=64;s.unicode=true;s.columns=120;s.lines=40;
    s.cloudSettings=s.clockSettings=s.terminalSettings=s.notificationSettings=true;
    std::string output;auto write=[&](const uint8_t *p,size_t n){output.append((const char*)p,n);};
    auto feed=[&](const char *keys){bool emitted=false;while(*keys)emitted=v.feed(uint8_t(*keys++),s,a)||emitted;return emitted;};
    v.begin();v.render(s,write);
    assert(output.find("Category")!=std::string::npos);assert(output.find("[x] On")!=std::string::npos);
    assert(output.find("1-21 / 64")!=std::string::npos);widths(output,119);
    assert(feed(" ")&&std::string(a.value)=="false");assert(std::string(rows[0].value)=="true");
    rows[0].pending=true;assert(!feed(" "));rows[0].pending=false;
    assert(!feed("\x1b[B"));assert(feed("+")&&std::string(a.value)=="129");
    assert(!feed("\r\x15" "256"));assert(!feed("\r")&&v.editing());
    output.clear();v.render(s,write);assert(output.find("Invalid value")!=std::string::npos);
    feed("\x15" "200");rows[1].value="130";assert(!feed("\r")&&v.editing());
    output.clear();v.render(s,write);assert(output.find("State changed")!=std::string::npos);
    feed("\x1b");v.flushEscape(a);assert(!v.editing());
    feed("\r\x15" "200");assert(feed("\r")&&std::string(a.value)=="200");
    feed("\x1b[B");assert(feed(" ")&&std::string(a.value)=="static");
    feed("\x1b[12~");assert(v.editing());output.clear();v.render(s,write);
    assert(output.find("Enter apply   Esc cancel")!=std::string::npos);
    feed("\x1b");v.flushEscape(a);
    for(unsigned theme=0;theme<4;++theme){v.setTheme(V::Theme(theme));output.clear();assert(v.render(s,write));assert(output.find("\x1b[48;")!=std::string::npos);}
    // Maximum viewport must be complete, bounded and use available space.
    s.columns=240;s.lines=100;output.clear();v.render(s,write);
    assert(output.size()<PS_DEVICE_VIEW_FRAME_BYTES);assert(output.find(" / 64")!=std::string::npos);widths(output,239);
    // Every navigation-only page transition is free of application actions.
    for(unsigned i=0;i<20;++i){assert(!feed("\t"));output.clear();v.render(s,write);assert(a.type==V::Action::None);}
    s.columns=40;s.lines=16;output.clear();v.render(s,write);widths(output,39);
    v.setAnsi(false);s.unicode=false;output.clear();v.render(s,write);assert(output.find('\x1b')==std::string::npos);widths(output,39);
    std::printf("Device table: typed controls, stale edits, dialogs, 4 palettes, 40..240 columns; state=%u bytes\n",unsigned(sizeof(v)));
}

#else
int main(){V v;V::Snapshot s;v.begin();v.render(s,[](const uint8_t*,size_t){});return 0;}
#endif
