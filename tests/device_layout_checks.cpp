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
    void tableBorders(unsigned left,bool unicode)const{
        const std::string vertical=unicode?"\xe2\x94\x82":"|";
        assert(cells[0][left]==(unicode?"\xe2\x95\xad":"+"));
        assert(cells[0][width-2]==(unicode?"\xe2\x95\xae":"+"));
        for(unsigned r=1;r<height-4;++r){
            assert(cells[r][left]==(r==2||r==4?(unicode?"\xe2\x94\x9c":"+"):vertical));
            assert(cells[r][width-2]==(r==2||r==4?(unicode?"\xe2\x94\xa4":"+"):vertical));
        }
        assert(cells[height-4][left]==(unicode?"\xe2\x95\xb0":"+"));
        assert(cells[height-4][width-2]==(unicode?"\xe2\x95\xaf":"+"));
        // Discover actual header separators, then require connected junctions
        // and uninterrupted rules through section, value and blank rows.
        unsigned separators=0;
        for(unsigned col=left+1;col<width-2;++col)if(cells[3][col]==vertical){
            ++separators;
            assert(cells[2][col]==(unicode?"\xe2\x94\xac":"+"));
            assert(cells[4][col]==(unicode?"\xe2\x94\xbc":"+"));
            for(unsigned r=5;r<height-4;++r)assert(cells[r][col]==vertical);
            assert(cells[height-4][col]==(unicode?"\xe2\x94\xb4":"+"));
        }
        assert(separators==(PS_DEVICE_VIEW_CATEGORY_COLUMN&&width-1-left>=95?2U:1U));
        if(left)for(unsigned r=1;r<height-4;++r)assert(cells[r][0]==vertical&&cells[r][19]==vertical);
    }
};

int main(){
#if PS_DEVICE_VIEW_TABLE
    for(unsigned width:{40U,64U,80U,120U,240U})for(unsigned height:{16U,24U,40U,100U})
    for(bool vertical:{false,true})for(bool unicode:{false,true}){
        V view;V::Snapshot s;V::Action a;V::Row rows[32];
        for(unsigned i=0;i<32;++i){rows[i]={"Setting","setting","Observed",true};rows[i].category=i<1?"Lighting":"Connection";}
        s.rows=rows;s.rowCount=2;s.columns=uint16_t(width);s.lines=uint16_t(height);s.unicode=unicode;
        s.terminalSettings=s.cloudSettings=s.notificationSettings=s.clockSettings=true;
        view.begin();view.setTabLayout(vertical?V::VerticalTabs:V::HorizontalTabs);
        Screen screen(width,height);std::string output;
        auto render=[&](bool force){output.clear();view.render(s,[&](const uint8_t *p,size_t n){output.append((const char*)p,n);},force);assert(output.size()<PS_DEVICE_VIEW_FRAME_BYTES);screen.feed(output);};
        auto feed=[&](const char *keys){while(*keys)view.feed(uint8_t(*keys++),s,a);};
        render(true);
        const unsigned left=vertical&&PS_DEVICE_VIEW_VERTICAL_TABS&&width>=64?22:0;
        screen.tableBorders(left,unicode);
        assert(screen.cells[0][left]==(unicode?"\xe2\x95\xad":"+"));
        assert(screen.cells[height-4][width-2]==(unicode?"\xe2\x95\xaf":"+"));
        assert(screen.cells[height-5][left]==(unicode?"\xe2\x94\x82":"|"));
        assert(screen.line(height-1).find("Tab pages")!=std::string::npos);
        assert(screen.line(height-2).find("select")!=std::string::npos);
        if(left)assert(screen.line(2).find("[Device]")!=std::string::npos);
        feed("\x1b[B");render(false);
        screen.tableBorders(left,unicode);
        auto incremental=screen.cells;render(true);assert(incremental==screen.cells);
        feed("\r\x15" "Draft");render(false);assert(view.editing());
        assert(screen.line(height-2).find("apply")!=std::string::npos);
        assert(screen.line(height-1).find("Tab pages")==std::string::npos);
        feed("\x1b");view.flushEscape(a);render(false);
        screen.tableBorders(left,unicode);
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
        screen.tableBorders(left,unicode);
        assert(screen.line(height-1).find("Tab pages")!=std::string::npos);
        assert(screen.cells[height-4][width-2]==(unicode?"\xe2\x95\xaf":"+"));
        s.rowCount=1;render(false);
        screen.tableBorders(left,unicode);
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
        assert(screen.cells[height-4][width-2]==(unicode?"\xe2\x95\xaf":"+"));
    }
#else
    for(unsigned width:{40U,80U,100U})for(unsigned height:{16U,24U,40U,100U})
    for(bool unicode:{false,true}){
        V view;V::Snapshot s;V::Row row("Setting","setting","Observed",true);
        s.rows=&row;s.rowCount=1;s.columns=uint16_t(width);s.lines=uint16_t(height);s.unicode=unicode;
        view.begin();Screen screen(width,height);std::string output;
        view.render(s,[&](const uint8_t *p,size_t n){output.append((const char*)p,n);},true);
        screen.feed(output);
        assert(output.size()<PS_DEVICE_VIEW_FRAME_BYTES);
        assert(output.find("\x1b[48;2;")==std::string::npos);
        assert(output.find("\x1b[48;5;")==std::string::npos);
        if(screen.cells[height-4][width-2]!=(unicode?"\xe2\x95\xaf":"+"))
            std::fprintf(stderr,"Incomplete retained frame %ux%u unicode=%d bytes=%zu\n",width,height,int(unicode),output.size());
        const std::string vertical=unicode?"\xe2\x94\x82":"|";
        for(unsigned r=3;r<height-4;++r)assert(screen.cells[r][0]==vertical&&screen.cells[r][width-2]==vertical);
        assert(screen.cells[height-4][0]==(unicode?"\xe2\x95\xb0":"+"));
        assert(screen.cells[height-4][width-2]==(unicode?"\xe2\x95\xaf":"+"));
        assert(screen.line(height-2).find("select")!=std::string::npos||width<60);
    }
#endif
}
