#include "portable_device_view.hpp"
#include <cassert>
#include <string>
using V=PortableShell::DeviceView;
int main(){
#if PS_DEVICE_VIEW_TABLE && PS_DEVICE_VIEW_SECTIONS
    V view;V::Snapshot s;V::Action a;
    V::Row rows[]={{"First","first","one",true},{"Second","second","two",true},
                   {"Third","third","three",true},{"Loose","loose","four",true}};
    rows[0].category=rows[1].category="Alpha";rows[2].category="Beta";
    s.rows=rows;s.rowCount=4;s.columns=100;s.lines=24;s.unicode=true;
    view.begin();view.setGroupLayout(V::SectionRows);
    auto keys=[&](const char *text){while(*text){assert(!view.feed(uint8_t(*text++),s,a));assert(a.type==V::Action::None);}};
    auto frame=[&](){std::string out;view.render(s,[&](const uint8_t*p,size_t n){out.append((const char*)p,n);},true);return out;};
    assert(!view.sectionBottomBorder());
    const std::string without=frame();assert(without.find("ALPHA")!=std::string::npos);
    view.setSectionBottomBorder(true);const std::string with=frame();
    assert(view.sectionBottomBorder()&&with!=without);
    view.setSectionBottomBorder(false);
    keys("\x1b[A\r\n");assert(view.sectionSelected());
    assert(frame().find("First")==std::string::npos);
    keys("\x1b[D\x1b[D");assert(frame().find("Second")==std::string::npos);
    keys("\x1b[B "); // Collapse Beta too.
    assert(frame().find("Third")==std::string::npos);
    keys("\x1b[B\r");assert(view.editing()&&std::string(view.draft())=="four");
    view.setGroupLayout(V::CategoryColumn);assert(view.groupLayout()==V::SectionRows);
    keys("\x1b");view.flushEscape(a);
    keys("\t\x1b[Z");assert(frame().find("First")==std::string::npos);
    keys("\x1b[H\x1b[C\x1b[C");assert(frame().find("First")!=std::string::npos);
    keys("\x1b[B\r");assert(view.editing()&&std::string(view.draft())=="one");
    keys("\x1b");view.flushEscape(a);
    view.setGroupLayout(V::CategoryColumn);assert(frame().find("Category")!=std::string::npos);
    view.setGroupLayout(V::SectionRows);keys("\x1b[H ");
    assert(frame().find("First")==std::string::npos);
    rows[0].key="replacement";assert(frame().find("First")!=std::string::npos);
    keys("\x1b[H ");s.unicode=false;
    assert(frame().find("+ ALPHA")!=std::string::npos);
#endif
}
