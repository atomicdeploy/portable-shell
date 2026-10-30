#include "portable_device_view.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
using V=PortableShell::DeviceView;

// Replay cursor movement and erase operations. Fail on wrap/scroll: a screenshot
// alone can hide a newline emitted at the bottom of the physical terminal.
struct Screen {
    unsigned width,height,row=0,column=0;std::string sequence;bool escape=false;
    std::vector<std::vector<std::string>> cells;
    Screen(unsigned w,unsigned h):width(w),height(h),cells(h,std::vector<std::string>(w," ")){}
    void feed(const std::string &bytes){
        for(size_t i=0;i<bytes.size();++i){unsigned char c=bytes[i];
            if(c==27){escape=true;sequence.clear();continue;}
            if(escape){
                if(c<0x40||c>0x7e||c=='['||c=='?'){sequence+=char(c);continue;}
                unsigned a=0,b=1;std::sscanf(sequence.c_str(),"[%u;%u",&a,&b);
                if(c=='H'){row=(a?a:1)-1;column=b-1;}
                if(c=='G')column=(a?a:1)-1;
                if(c=='J'||c=='K')for(unsigned r=0;r<height;++r)for(unsigned x=0;x<width;++x)
                    if(c=='J'?(a==2||r>row||(r==row&&x>=column)):(r==row&&(a==2||x>=column)))cells[r][x]=" ";
                escape=false;continue;
            }
            if(c=='\r'){column=0;continue;}
            if(c=='\n'){++row;assert(row<height);continue;}
            if(!(row<height&&column<width-1))std::fprintf(stderr,"Overflow %ux%u row=%u col=%u byte=%u near %.40s\n",width,height,row,column,unsigned(c),bytes.c_str()+i);
            assert(row<height&&column<width-1);
            const unsigned n=c<128?1:(c&0xe0)==0xc0?2:(c&0xf0)==0xe0?3:4;
            cells[row][column++]=bytes.substr(i,n);i+=n-1;
        }
    }
    std::string line(unsigned r)const{std::string result;for(const auto &c:cells[r])result+=c;return result;}
    void tableBorders(unsigned left,bool unicode,bool inside,bool compact)const{
        const unsigned right=width-2-(!compact&&width-left>=64?2:0);
        const unsigned top=inside?0:3, header=inside?3:4, body=header+2;
        const std::string vertical=unicode?"\xe2\x94\x82":"|";
        assert(cells[top][left]==(unicode?"\xe2\x95\xad":"+"));
        assert(cells[top][right]==(unicode?"\xe2\x95\xae":"+"));
        for(unsigned r=top+1;r<height-4;++r){
            const bool rule=cells[r][left]==(unicode?"\xe2\x94\x9c":"+");
            assert(cells[r][left]==(rule?(unicode?"\xe2\x94\x9c":"+"):vertical));
            if(cells[r][right]!=(rule?(unicode?"┤":"+"):vertical)&&cells[r][right]!=(unicode?(rule?"┫":"┃"):"#"))std::fprintf(stderr,"Bad edge %ux%u inside=%d left=%u row=%u: %s\n",width,height,int(inside),left,r,line(r).c_str());
            assert(cells[r][right]==(rule?(unicode?"\xe2\x94\xa4":"+"):vertical)||cells[r][right]==(unicode?(rule?"┫":"┃"):"#"));
            if(rule)for(unsigned c=left+1;c<right;++c)
                assert(cells[r][c]==(unicode?"─":"-")||cells[r][c]==(unicode?"┬":"+")||cells[r][c]==(unicode?"┴":"+")||cells[r][c]==(unicode?"┼":"+"));
        }
        assert(cells[height-4][left]==(unicode?"\xe2\x95\xb0":"+"));
        assert(cells[height-4][right]==(unicode?"\xe2\x95\xaf":"+"));
        // Discover actual header separators, then require connected junctions
        // and uninterrupted rules through section, value and blank rows.
        unsigned separators=0;
        for(unsigned col=left+1;col<right;++col)if(cells[header][col]==vertical){
            ++separators;
            assert(cells[header-1][col]==(unicode?"\xe2\x94\xac":"+"));
            assert(cells[header+1][col]==(unicode?"┼":"+")||cells[header+1][col]==(unicode?"┴":"+"));
            for(unsigned r=body;r<height-4;++r){
                const auto &marker=cells[r][left+2];
                const bool section=marker=="▾"||marker=="▸"||marker=="-"||marker=="+";
                const bool rule=cells[r][left]==(unicode?"├":"+");
                if(!section&&!rule)assert(cells[r][col]==vertical);
                if(rule&&unicode){
                    const bool above=cells[r-1][col]==vertical;
                    const bool below=cells[r+1][col]==vertical;
                    assert(cells[r][col]==(above?(below?"┼":"┴"):(below?"┬":"─")));
                }
            }
            assert(cells[height-4][col]==(unicode?(cells[height-5][col]==vertical?"┴":"─"):"+"));
        }
        assert(separators==(PS_DEVICE_VIEW_CATEGORY_COLUMN&&right+1-left>=95?2U:1U));
        if(left)for(unsigned r=1;r<height-4;++r)assert(cells[r][0]==vertical&&cells[r][19]==vertical);
    }
};

static void adaptiveGutter(){
    V view;V::Snapshot s;V::Row rows[32];V::Action a;
    for(auto &r:rows)r={"Item","item","value",true};
    s.rows=rows;s.rowCount=1;s.columns=100;s.lines=24;s.unicode=true;
    view.begin();Screen screen(100,24);
    auto paint=[&](bool force){std::string out;view.render(s,[&](const uint8_t *p,size_t n){out.append((const char*)p,n);},force);screen.feed(out);};
    paint(true);assert(screen.cells[3][98]=="╮"); // No gutter for a fitting table.
    s.rowCount=32;paint(false);assert(screen.cells[3][96]=="╮");
    auto incremental=screen.cells;paint(true);assert(incremental==screen.cells);
    s.rowCount=1;paint(false);assert(screen.cells[3][98]=="╮");
    incremental=screen.cells;paint(true);assert(incremental==screen.cells);
    view.setReserveScrollbar(true);paint(false);assert(screen.cells[3][96]=="╮");
    view.setReserveScrollbar(false);paint(false);assert(screen.cells[3][98]=="╮");
    (void)a;
}
int main(){
    adaptiveGutter();
    for(unsigned width:{40U,64U,80U,120U,240U})for(unsigned height:{16U,24U,40U,100U})
    for(bool compact:{false,true})for(bool vertical:{false,true})for(bool unicode:{false,true})for(bool borders:{false,true})for(bool inside:{false,true}){
        V view;view.setReserveScrollbar(true);view.setCompactScrollbar(compact);V::Snapshot s;V::Action a;V::Row rows[32];
        for(unsigned i=0;i<32;++i){rows[i]={"Setting","setting","Observed",true};rows[i].category=i<1?"Lighting":"Connection";}
        s.rows=rows;s.rowCount=2;s.columns=uint16_t(width);s.lines=uint16_t(height);s.unicode=unicode;
        s.terminalSettings=s.cloudSettings=s.notificationSettings=s.clockSettings=true;
        view.begin();view.setTabLayout(vertical?V::VerticalTabs:V::HorizontalTabs);
        view.setSectionBorders(borders);view.setTabsInsideTable(inside);
        const unsigned top=inside?0:3;
        Screen screen(width,height);std::string output;
        auto render=[&](bool force){output.clear();view.render(s,[&](const uint8_t *p,size_t n){output.append((const char*)p,n);},force);assert(output.size()<PS_DEVICE_VIEW_FRAME_BYTES);screen.feed(output);};
        auto feed=[&](const char *keys){while(*keys)view.feed(uint8_t(*keys++),s,a);};
        render(true);
        const unsigned left=vertical&&PS_DEVICE_VIEW_VERTICAL_TABS&&width>=64?22:0;
        const unsigned right=width-2-(!compact&&width-left>=64?2:0);
        screen.tableBorders(left,unicode,inside,compact);
        assert(screen.cells[top][left]==(unicode?"\xe2\x95\xad":"+"));
        assert(screen.cells[height-4][right]==(unicode?"\xe2\x95\xaf":"+"));
        assert(screen.line(height-1).find("Tab pages")!=std::string::npos);
        assert(screen.line(height-2).find("select")!=std::string::npos);
        if(left)assert(screen.line(2).find(PS_TAB_BRACKETS?"[Device]":" Device ")!=std::string::npos);
#if PS_DEVICE_VIEW_SECTIONS && !PS_DEVICE_VIEW_CATEGORY_COLUMN
        feed("\x1b[A");render(false);assert(view.sectionSelected());
        auto heading=screen.cells;render(true);assert(heading==screen.cells);
        feed(" ");render(false);screen.tableBorders(left,unicode,inside,compact);
        auto folded=screen.cells;render(true);assert(folded==screen.cells);
        feed("\x1b[C\x1b[B");render(false);assert(!view.sectionSelected());
#endif
        feed("\x1b[B");render(false);
        if(view.sectionSelected()){feed("\x1b[B");render(false);}
        screen.tableBorders(left,unicode,inside,compact);
        auto incremental=screen.cells;render(true);assert(incremental==screen.cells);
        feed("\r\x15" "Draft");render(false);assert(view.editing());
        assert(screen.line(height-2).find("apply")!=std::string::npos);
        assert(screen.line(height-1).find("Tab pages")==std::string::npos);
        feed("\x1b");view.flushEscape(a);render(false);
        screen.tableBorders(left,unicode,inside,compact);
#if PS_DEVICE_VIEW_DIALOGS
        feed("\x1b[12~");render(false);assert(view.editing());
        assert(screen.line(height/2-1).find("Enter apply")!=std::string::npos);
        assert(screen.line(height-1).find("Tab pages")==std::string::npos);
        feed("\x1b");view.flushEscape(a);render(false);
#endif
        feed("\t");render(false);assert(view.page()==V::Network&&a.type==V::Action::None);
        feed("\x1b[Z");render(false);assert(view.page()==V::Device&&a.type==V::Action::None);
        // Shrinking populated content must erase old rows, including sidebar updates.
        s.rowCount=32;render(true);
        screen.tableBorders(left,unicode,inside,compact);
        const unsigned presentation=32+(PS_DEVICE_VIEW_SECTIONS&&!PS_DEVICE_VIEW_CATEGORY_COLUMN?(borders?3:2):0);
        if(presentation>height-9-(inside?0:1)){
            feed("\x1b[F");render(false);
            if(!compact&&width-left>=64)for(unsigned r=inside?5:6;r<height-4;++r)assert(screen.cells[r][right+1]==" ");
            const auto &last=screen.cells[height-5][width-2];
            assert(last==(unicode?"┃":"#")||last==(unicode?"┫":"#"));
            feed("\x1b[H");render(false);
            const auto &first=screen.cells[inside?5:6][width-2];
            assert(first==(unicode?"┃":"#")||first==(unicode?"┫":"#"));
            if(view.sectionSelected())feed("\x1b[B");
        }
        assert(screen.line(height-1).find("Tab pages")!=std::string::npos);
        assert(screen.cells[height-4][right]==(unicode?"\xe2\x95\xaf":"+"));
        s.rowCount=1;render(false);
        screen.tableBorders(left,unicode,inside,compact);
        incremental=screen.cells;render(true);assert(incremental==screen.cells);
        s.cloudSettings=s.notificationSettings=s.clockSettings=false;render(false);
        incremental=screen.cells;render(true);assert(incremental==screen.cells);
        // Keep a draft across a resize that changes the sidebar fallback.
        feed("\r\x15" "Resize");render(false);
        s.columns=width==40?80:40;screen=Screen(s.columns,height);render(false);
        assert(std::string(view.draft())=="Resize");
        assert(screen.line(height-2).find("apply")!=std::string::npos);
        feed("\x1b");view.flushEscape(a);s.columns=uint16_t(width);screen=Screen(width,height);render(false);
        // Matrix and settings pages share the same window footer in both layouts.
        s.canLeds=true;uint8_t states[80]={};s.cellStates=states;
        feed("\t\t");render(false);assert(view.page()==V::Led);
        assert(screen.line(height-1).find("Tab pages")!=std::string::npos);
        assert(screen.cells[height-4-(PS_DEVICE_VIEW_MATRIX?0:height<24?5:9)][right]==(unicode?"\xe2\x95\xaf":"+"));
    }
}
