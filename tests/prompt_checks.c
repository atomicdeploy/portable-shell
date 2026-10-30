#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
unsigned ps_replay_row(void);
unsigned ps_replay_column(void);
const char *ps_replay_cell(unsigned,unsigned);
static union {void *align;unsigned char bytes[40000];} memory;
static char output[131072],svg[400000];static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;
    ps_replay_feed(p,n);
}
static void keys(ps_session *s,const char *p){while(*p)ps_key(s,(unsigned char)*p++);}
static ps_session *session(unsigned flags){
    ps_config c={sink,0,96,24,flags};used=0;output[0]=0;ps_replay_reset();
    return ps_init(memory.bytes,sizeof memory.bytes,&c);
}
static int save(const char *p){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(p,"wb");int bad;
    if(!f)return 1;
    bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
}
int main(int argc,char **argv){
    ps_session *s=session(PS_INTERACTIVE|PS_UNICODE|PS_COLOR);
    ps_prompt_part parts[]={{"workbench",PS_STYLE_SUCCESS},{" /config\n",PS_STYLE_MUTED},{"❯ ",PS_STYLE_COMMAND}};
    /* Use real LF in the API; command parser also supports quoted backslash-n. */
    parts[1].text=" /config\n";
    assert(ps_set_prompt_parts(s,parts,3)==0);ps_start(s);
    assert(ps_replay_row()==1&&ps_replay_column()==2);
    keys(s,"abc");assert(ps_replay_row()==1&&ps_replay_column()==5);
    ps_key(s,PS_KEY_LEFT);assert(ps_replay_column()==4);
    ps_key(s,PS_KEY_HOME);assert(ps_replay_column()==2);
    ps_key(s,PS_KEY_END);assert(ps_replay_column()==5);
    ps_key(s,8);assert(ps_replay_column()==4);
    assert(ps_set_prompt(s,"bad\033[2J",PS_STYLE_DEFAULT)==-1);
    assert(ps_set_prompt(s,"1\n2\n3\n4\n5",0)==-1);
    assert(ps_set_prompt_parts(s,0,1)==-1);
    assert(ps_set_prompt(s,"$ ",PS_STYLE_SUCCESS)==0);
    assert(ps_replay_row()==0&&ps_replay_column()==4);
    assert(!strcmp(ps_replay_cell(1,0)," "));
    assert(ps_set_prompt_parts(s,parts,3)==0);
    ps_resize(s,12,4);assert(ps_replay_row()==1&&ps_replay_column()<12);
    ps_key(s,12);assert(ps_replay_row()==1);
    ps_key(s,3);assert(ps_replay_row()==3);
    s=session(PS_INTERACTIVE|PS_UNICODE|PS_COLOR);
    ps_set_prompt_parts(s,parts,3);ps_start(s);ps_history_add(s,"echo hello");
    keys(s,"echo h");assert(ps_replay_row()==1&&ps_replay_column()==8);
    ps_key(s,PS_KEY_END);assert(ps_replay_row()==1&&ps_replay_column()==12);
    ps_key(s,21);ps_key(s,PS_KEY_UP);assert(ps_replay_column()==12);
    ps_key(s,PS_KEY_DOWN);assert(ps_replay_column()==2);
    assert(ps_set_prompt(s,"one\ntwo\nthree\n$ ",PS_STYLE_BLUE)==0);
    assert(strstr(output,"\033[94m"));
    ps_resize(s,12,4);assert(ps_replay_column()==2);
    ps_key(s,12);assert(ps_replay_row()==2);
    assert(!strcmp(ps_replay_cell(0,0),"t"));
    {
        char large[257];memset(large,'x',sizeof large);large[255]=0;
        assert(ps_set_prompt(s,large,PS_STYLE_DEFAULT)==0);
        large[255]='x';large[256]=0;
        assert(ps_set_prompt(s,large,PS_STYLE_DEFAULT)==-1);
    }
    s=session(0);assert(ps_set_prompt_parts(s,parts,3)==0);ps_start(s);
    assert(!used);ps_execute(s,"echo batch");assert(!strchr(output,27));
    s=session(PS_INTERACTIVE);ps_set_prompt(s,"host\n$ ",PS_STYLE_SUCCESS);ps_start(s);
    assert(!strstr(output,"mhost")); /* Monochrome still uses cursor controls, no colors. */
    assert(ps_replay_row()==1&&ps_replay_column()==2);
    s=session(PS_INTERACTIVE|PS_COLOR|PS_UNICODE);ps_start(s);
    assert(ps_execute(s,"prompt success \"host\\n\" command \"❯ \"")==0);
    ps_key(s,'x');assert(ps_replay_row()==1&&ps_replay_column()==3);
    if(argc==1)return 0;
    if(argc!=4)return 2;
    s=session(PS_INTERACTIVE|PS_COLOR|PS_UNICODE);
    parts[0].text="operator@workbench";parts[1].text="  /configuration  ";
    parts[2].text="❯ ";ps_set_prompt_parts(s,parts,3);ps_start(s);
    keys(s,"echo ready");ps_key(s,13);keys(s,"help");if(save(argv[1]))return 3;
    s=session(PS_INTERACTIVE|PS_COLOR|PS_UNICODE);
    parts[0].text="╭─ operator@workbench";parts[1].text="  /configuration\n";
    parts[2].text="╰─❯ ";ps_set_prompt_parts(s,parts,3);ps_start(s);
    keys(s,"echo ready");ps_key(s,13);keys(s,"help");if(save(argv[2]))return 4;
    s=session(PS_INTERACTIVE);ps_set_prompt(s,"operator@workbench /configuration\n-> ",PS_STYLE_DEFAULT);ps_start(s);
    keys(s,"echo ready");ps_key(s,13);keys(s,"help");if(save(argv[3]))return 5;
    return 0;
}
