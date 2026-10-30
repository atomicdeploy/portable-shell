#include "portable_device_view.hpp"
#include <cassert>
#include <string>
#include <vector>

// Compare terminal cells, not UTF-8 byte counts or a hand-drawn screenshot.
static std::vector<std::vector<std::string> > lines(const std::string &bytes) {
    std::vector<std::vector<std::string> > result(1);
    for (size_t i=0;i<bytes.size();) {
        unsigned char c=bytes[i++];
        if(c==27){assert(i<bytes.size()&&bytes[i++]=='[');
            while(i<bytes.size()){unsigned char end=bytes[i++];if(end>=0x40&&end<=0x7e)break;}
        }else if(c=='\n'){result.push_back(std::vector<std::string>());}
        else if(c!='\r'){
            unsigned n=c<128?1:(c&0xe0)==0xc0?2:(c&0xf0)==0xe0?3:4;
            assert(i+n-1<=bytes.size());result.back().push_back(bytes.substr(i-1,n));i+=n-1;
        }
    }
    return result;
}
int main(){
    using V=PortableShell::DeviceView;
    const unsigned widths[]={40,79,80,96,100,240};
    const char *titles[]={"Console","A very long device identity that must be clipped at the same cell used to measure the closing border, even in narrow terminals"};
    for(unsigned width:widths)for(bool unicode:{false,true})for(bool ansi:{false,true})for(const char *title:titles){
        V view;V::Snapshot s;V::Action a;
        s.columns=uint16_t(width);s.lines=24;s.unicode=unicode;s.title=title;
        s.canLeds=s.canPixels=s.canGame=s.terminalSettings=s.cloudSettings=s.notificationSettings=s.clockSettings=true;
        s.pixelCount=480;view.begin(ansi);
        for(unsigned page=0;page<V::PageCount;++page){
            std::string output;view.render(s,[&](const uint8_t *p,size_t n){output.append((const char*)p,n);},true);
            const auto rows=lines(output);assert(rows.size()>=3);
            const unsigned expected=(PS_DEVICE_VIEW_TABLE?width:width>100?100:width)-1;
            for(unsigned row=0;row<3;++row){
                assert(rows[row].size()==expected);
                assert(rows[row].front()==(unicode?(row==0?"\xe2\x95\xad":row==1?"\xe2\x94\x82":"\xe2\x94\x9c"):(row==1?"|":"+")));
                assert(rows[row].back()==(unicode?(row==0?"\xe2\x95\xae":row==1?"\xe2\x94\x82":"\xe2\x94\xa4"):(row==1?"|":"+")));
            }
            assert(!view.feed('\t',s,a));assert(a.type==V::Action::None);
        }
    }
}
