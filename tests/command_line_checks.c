#include "portable_command_line.h"
#include "portable_shell.h"
#include <assert.h>
#include <string.h>
static const char *lookup(void *ctx,const char *name){
    (void)ctx;if(!strcmp(name,"TEXT"))return "two words; echo injected";
    if(!strcmp(name,"?"))return "7";
    return "";
}
static ps_shell_plan p;
static union{void *align;unsigned char bytes[65536];} storage;
static char output[2048];static unsigned used,lines,raw;
static void sink(void *ctx,const ps_byte *b,unsigned n){(void)ctx;assert(used+n<sizeof output);memcpy(output+used,b,n);used+=n;output[used]=0;}
static int provider(ps_session *s,const char *line,void *ctx){
    const char *argv[]={"echo","callback"};(void)ctx;
    assert(!strcmp(line,"custom syntax"));++lines;return ps_execute_argv(s,2,argv);
}
static int raw_command(ps_session *s,const char *tail,void *ctx){(void)s;(void)ctx;assert(!strcmp(tail,"unparsed | ;"));++raw;return 3;}
static unsigned complete(ps_session *s,int argc,const char *const *argv,const char *prefix,const char **items,unsigned capacity,void *ctx){
    (void)s;(void)argc;(void)argv;(void)prefix;(void)ctx;
    if(capacity){items[0]="safe;$TEXT";return 1;}return 0;
}
static int literal(ps_session *s,int argc,const char *const *argv,void *ctx){
    (void)s;(void)ctx;assert(argc==2&&!strcmp(argv[1],"safe;$TEXT"));return 0;
}
static int parsed(ps_session *s,const char *line,void *ctx){
    (void)ctx;assert(ps_shell_parse(line,&p,lookup,0)&&p.count==1);
    return ps_execute_argv(s,p.commands[0].argc,p.commands[0].argv);
}
static int metadata(ps_session *s,int n,const char *const *a,void *c){(void)s;(void)n;(void)a;(void)c;return 0;}
int main(void){
    ps_config config={sink,0,80,24,0};ps_session *s;ps_write_fn other=sink;void *context=&lines;
    ps_command command={"raw","TAIL","Test",metadata,0,0};
    assert(ps_shell_parse("echo \"a b\" '' '$TEXT' \"$TEXT\" $? # rest",&p,lookup,0));
    assert(p.count==1 && p.commands[0].argc==6);
    assert(!strcmp(p.commands[0].argv[2],""));
    assert(!strcmp(p.commands[0].argv[3],"$TEXT"));
    assert(!strcmp(p.commands[0].argv[4],"two words; echo injected"));
    assert(!strcmp(p.commands[0].argv[5],"7"));
    assert(ps_shell_parse("one < in | two > out 2>&1 && yes || no ; last;",&p,0,0));
    assert(p.count==5&&p.commands[0].next==PS_CHAIN_PIPE&&p.commands[1].merge_error);
    assert(p.commands[1].next==PS_CHAIN_AND&&p.commands[2].next==PS_CHAIN_OR);
    assert(ps_shell_parse("echo a\\ b 'C:\\folder\\file' \">\"",&p,0,0));
    assert(!strcmp(p.commands[0].argv[1],"a b")&&!strcmp(p.commands[0].argv[2],"C:\\folder\\file"));
    assert(!ps_shell_parse("echo before; | bad",&p,0,0));
    assert(!ps_shell_parse("echo x &&",&p,0,0));
    assert(!ps_shell_parse("echo x & other",&p,0,0));
    assert(!ps_shell_parse("echo $(bad)",&p,0,0));
    assert(!ps_shell_parse("echo 'bad",&p,0,0));
    assert(!ps_shell_parse("echo x > one > two",&p,0,0));
    assert(!ps_shell_parse("echo x 2>&1 > late",&p,0,0));
    assert(!ps_shell_parse("echo x << input",&p,0,0));
    {
        char many[100];unsigned i;
        for(i=0;i<34;++i){many[i*2]='a';many[i*2+1]=' ';}
        many[68]=0;assert(!ps_shell_parse(many,&p,0,0));
        for(i=0;i<17;++i){many[i*2]='a';many[i*2+1]=';';}
        many[34]=0;assert(!ps_shell_parse(many,&p,0,0));
    }
    s=ps_init(storage.bytes,sizeof storage.bytes,&config);assert(s);
    ps_set_line_executor(s,provider,0);assert(ps_execute(s,"custom syntax")==0&&lines==1);
    assert(ps_register_raw(s,&command,raw_command)==0);
    assert(ps_execute(s,"raw unparsed | ;")==3&&raw==1&&lines==1);
    ps_exchange_output(s,&other,&context);assert(other==sink&&context==0);
    ps_exchange_output(s,&other,&context);assert(context==&lines);
    ps_set_line_executor(s,0,0);assert(ps_execute(s,"echo original")==0);
    ps_set_status(s,17);assert(ps_status(s)==17);
    {
        ps_command c={"argument","VALUE","Test",literal,complete,0};
        const char *keys="argument s\t\r";
        config.flags=PS_INTERACTIVE|PS_NO_SUGGESTIONS;s=ps_init(storage.bytes,sizeof storage.bytes,&config);
        assert(ps_register(s,&c)==0);ps_set_line_executor(s,parsed,0);
        ps_feed(s,keys,(unsigned)strlen(keys));assert(ps_status(s)==0);
    }
    return 0;
}
