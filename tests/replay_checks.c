#include <assert.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char svg[400000];
int main(void) {
    const unsigned char bytes[]="\033[H\033[2J\033[31m\xe2\x95\xad\xe2\x94\x80\xe2\x95\xae\r\n\xe2\x94\x82X\xe2\x94\x82";
    unsigned i,n;
    ps_replay_reset();
    for(i=0;i<sizeof(bytes)-1;++i)ps_replay_feed(bytes+i,1);
    n=ps_replay_svg(svg);assert(n<sizeof(svg));svg[n]=0;
    assert(strstr(svg,"font-family=\"Agave\""));
    assert(strstr(svg,">X</text>"));
    assert(strstr(svg,"v -7 q 0 -6 6 -6"));
    assert(strstr(svg,"v 26"));
    ps_replay_reset();n=ps_replay_svg(svg);svg[n]=0;
    assert(!strstr(svg,">X</text>"));
    /* Ghost suggestions save/restore the insertion cursor, then overwrite the
       suggestion as each key arrives. A missing restore renders HELPELPLPP. */
    {const unsigned char prompt[]="H\033[sELP\033[uE\033[sLP\033[uL\033[sP\033[uP\033[K";
     ps_replay_feed(prompt,sizeof(prompt)-1);n=ps_replay_svg(svg);svg[n]=0;
     assert(strstr(svg,"x=\"68\" y=\"52\""));
     assert(!strstr(svg,"x=\"80\" y=\"52\""));}
    return 0;
}
