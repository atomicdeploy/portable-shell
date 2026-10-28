/* Host-only picture fixture. Renders the exact emitted terminal byte stream. */
#include "portable_device_view.hpp"
#include <stdio.h>
extern "C" void ps_replay_reset(void);
extern "C" void ps_replay_feed(const unsigned char *,unsigned);
extern "C" unsigned ps_replay_svg(char *);
static char svg[400000];
static int save(const char *path){
    unsigned count=ps_replay_svg(svg);FILE *f=fopen(path,"wb");if(!f)return 1;
    int bad=fwrite(svg,1,count,f)!=count;return fclose(f)||bad;
}
int main(int argc,char **argv){
    if(argc<3)return 2;
    using View=PortableShell::DeviceView;
    View view;View::Snapshot snapshot;View::Action action;
    View::Row rows[]={{"Hostname","hostname","workbench",true,false},
        {"Label","label","PortableShell device profile",true,false},
        {"Link","link","USB / acknowledged",false,false},
        {"Output","output","On",true,false},
        {"PWM","pwm","42%",true,false},
        {"Credentials","password","never-visible",true,true},
        {"Firmware","firmware","0.8.0",false,false}};
    rows[3].value="true";rows[3].kind=View::Toggle;rows[4].value="128";rows[4].kind=View::Level;rows[4].minimum=1;rows[4].maximum=255;
    for(auto &r:rows)r.category="Device";
    snapshot.title="PortableShell / device workspace";snapshot.rows=rows;snapshot.rowCount=7;
    snapshot.unicode=true;snapshot.columns=96;snapshot.lines=24;
    snapshot.terminalSettings=snapshot.cloudSettings=snapshot.notificationSettings=snapshot.clockSettings=true;
    auto write=[](const unsigned char *b,size_t n){ps_replay_feed(b,static_cast<unsigned>(n));};
    ps_replay_reset();view.begin();if(argc>3)view.setTheme(View::Theme(argv[3][0]-'0'));view.render(snapshot,write);
    if(save(argv[1]))return 3;
    for(unsigned char c:"\x1b[B\r\x15" "Shared editor")if(c)view.feed(c,snapshot,action);
    view.render(snapshot,write);
    if(save(argv[2]))return 4;
    printf("ps_editor=%u bytes, ps_telnet=%u bytes, DeviceView=%u bytes; no heap\n",
        static_cast<unsigned>(sizeof(ps_editor)),static_cast<unsigned>(sizeof(ps_telnet)),static_cast<unsigned>(sizeof(View)));
    return 0;
}
