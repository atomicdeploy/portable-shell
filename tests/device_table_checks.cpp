#include "portable_device_view.hpp"
#include <cassert>
#include <cstdio>
#include <string>
using V=PortableShell::DeviceView;
#if PS_DEVICE_VIEW_TABLE && PS_DEVICE_VIEW_WIDGETS && PS_DEVICE_VIEW_DIALOGS && PS_STYLE_THEMES
static void widths(const std::string &output,unsigned maximum){
    unsigned column=0;bool escape=false;std::string sequence;
    for(unsigned char c:output){
        if(c==27){escape=true;sequence.clear();continue;}
        if(escape){
            if(c>=0x40&&c<=0x7e&&c!='['){
                unsigned row=1,col=1;
                if(c=='H'){std::sscanf(sequence.c_str(),"[%u;%u",&row,&col);column=col-1;}
                if(c=='G'){std::sscanf(sequence.c_str(),"[%u",&col);column=col-1;}
                escape=false;
            }else sequence+=char(c);
            continue;
        }
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
    assert(output.find("[Device]")==std::string::npos);
    assert(output.find(PS_KEYCAP_STYLE "Space")!=std::string::npos);
    v.setTabBrackets(true);v.setFooterKeycaps(false);output.clear();v.render(s,write);
    assert(output.find("[Device]")!=std::string::npos);
    assert(output.find(PS_KEYCAP_STYLE "Space")==std::string::npos);
    v.setTabBrackets(false);v.setFooterKeycaps(true);output.clear();v.render(s,write);
#if PS_MOUSE
    {
        V m;V::Snapshot ms=s;ms.rowCount=3;V::Action ma;
        m.setGroupLayout(V::CategoryColumn);m.begin();m.render(ms,write);
        auto send=[&](const char *p){bool hit=false;while(*p)hit=m.feed(uint8_t(*p++),ms,ma)||hit;return hit;};
        assert(send("\x1b[<0;80;7M")&&ma.type==V::Action::Set&&std::string(ma.value)=="false");
        assert(!send("\x1b[<0;80;7m"));
        assert(send("\x1b[<0;80;8M")&&ma.type==V::Action::Set);
        assert(send("\x1b[<32;90;8M")&&ma.type==V::Action::Set);
        send("\x1b[<0;90;8m");assert(!send("\x1b[<32;95;8M"));
        assert(!send("\x1b[<0;99999;8M"));
        rows[0].pending=true;assert(!send("\x1b[<0;80;7M"));rows[0].pending=false;
        send("\x1b[<0;16;3M");assert(m.page()==V::Network);
        m.setTabLayout(V::VerticalTabs);m.render(ms,write);send("\x1b[<0;6;3M");assert(m.page()==V::Device);
        m.render(ms,write);assert(!send("\x1b[<65;40;9M"));assert(ma.type==V::Action::None);
        output.clear();m.end(write);assert(output.find("\x1b[?1006l")!=std::string::npos&&!m.active());
    }
    output.clear();v.render(s,write,true);
#endif
    assert(v.theme()==V::Semantic);
    assert(output.find("\x1b[38;2;255;184;108m")!=std::string::npos); // Numeric control, not a theme accent.
    assert(output.find("\x1b[38;2;255;95;115m")==std::string::npos); // Ordinary borders are not alarms.
    rows[0].value="false";output.clear();v.render(s,write,true);
    assert(output.find("\x1b[38;2;151;160;175m\xe2\x97\x8b ")!=std::string::npos);
    rows[0].value="true";output.clear();v.render(s,write,true);
    assert(output.find("Category")==std::string::npos);assert(output.find("[x]")==std::string::npos);
#if PS_DEVICE_VIEW_SECTIONS
    assert(output.find("LIGHTING")!=std::string::npos);assert(output.find("LIGHTING",output.find("LIGHTING")+1)==std::string::npos);
#endif
    assert(output.find("1 / 64")!=std::string::npos);widths(output,119);
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
    assert(output.find("cursor  Enter apply")==std::string::npos);
    feed("\x1b");v.flushEscape(a);
    for(unsigned theme=0;theme<4;++theme){v.setTheme(V::Theme(theme));output.clear();assert(v.render(s,write));assert(output.find("\x1b[48;")!=std::string::npos);}
    // Maximum viewport must be complete, bounded and use available space.
    s.columns=240;s.lines=100;output.clear();v.render(s,write);
    assert(output.size()<PS_DEVICE_VIEW_FRAME_BYTES);assert(output.find(" / 64")!=std::string::npos);widths(output,239);
    // Every navigation-only page transition is free of application actions.
    for(unsigned i=0;i<20;++i){assert(!feed("\t"));output.clear();v.render(s,write);assert(a.type==V::Action::None);}
    s.columns=40;s.lines=16;output.clear();v.render(s,write);widths(output,39);
    v.setAnsi(false);s.unicode=false;output.clear();v.render(s,write);assert(output.find('\x1b')==std::string::npos);widths(output,39);
    // A matrix is not a generic property table plus duplicated action hints.
    V matrix;V::Snapshot board;uint8_t states[80];uint32_t colors[80];
    const uint32_t palette[]={0xffeecc,0xd82855,0,0x33dd99};
    for(unsigned i=0;i<80;++i){states[i]=uint8_t(i%4);colors[i]=palette[i%4];}
    board.cellStates=states;board.cellColors=colors;board.cellStaggered=true;board.canLeds=true;
    board.columns=96;board.lines=24;board.unicode=true;matrix.begin();
    matrix.feed('\t',board,a);matrix.feed('\t',board,a);
    output.clear();matrix.render(board,write);widths(output,95);
    assert(output.find("20 \xc3\x97 4 staggered")!=std::string::npos);
    assert(output.find("\x1b[48;2;51;221;153m")!=std::string::npos); // Configured won color, not gray.
    assert(output.find("\x1b[48;2;216;40;85m")!=std::string::npos); // Configured lost color, not fixed red.
    assert(output.find("Category")==std::string::npos&&output.find("idle")!=std::string::npos&&output.find("Tab pages")==output.rfind("Tab pages"));
    auto arrow=[&](char key){matrix.feed(27,board,a);matrix.feed('[',board,a);matrix.feed(key,board,a);assert(a.type==V::Action::None);};
    arrow('C');assert(matrix.selectedCell()==11);arrow('C');assert(matrix.selectedCell()==2);
    arrow('B');assert(matrix.selectedCell()==22);arrow('D');assert(matrix.selectedCell()==31);
    matrix.feed('f',board,a);assert(!matrix.feed('r',board,a)&&a.type==V::Action::None);
    matrix.feed('f',board,a);matrix.feedback("ERR owner rejected");output.clear();matrix.render(board,write);
    assert(output.find("ERR owner rejected")!=std::string::npos);
    arrow('F');assert(matrix.selectedCell()==80);
    board.columns=40;board.lines=16;output.clear();matrix.render(board,write);widths(output,39);
    assert(output.find("Tab next")==std::string::npos&&output.find("Tab pages")==output.rfind("Tab pages"));
    matrix.setAnsi(false);board.unicode=false;output.clear();matrix.render(board,write);widths(output,39);
    for(unsigned char c:output)assert(c<128&&c!=27);
    std::printf("Device table: typed controls, stale edits, dialogs, 4 palettes, 40..240 columns; state=%u bytes\n",unsigned(sizeof(v)));
}

#else
int main(){V v;V::Snapshot s;v.begin();v.render(s,[](const uint8_t*,size_t){});return 0;}
#endif
