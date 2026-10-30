#include "portable_shell.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static char output[262144],svg[800000];static unsigned used,completed,cancelled;
static union {void *alignment;unsigned char bytes[50000];} memory;
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);
}
static int save(const char *path){
    unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;
    if(!f)return 1;
    bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;
}
static void done(ps_session *s,void *ctx,int accepted){
    (void)s;(void)ctx;if(accepted)++completed;else ++cancelled;
}
static const char *validate(void *ctx,unsigned index,const char *value){
    (void)ctx;(void)index;
    return strlen(value)>20?"The answer must not be longer than 20 characters.":0;
}
static void keys(ps_session *s,const char *text){used=0;ps_feed(s,text,(unsigned)strlen(text));}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE};
    char name[64]="",role[64]="Member";const char *choices[]={"Member","Contributor","Owner"};
    ps_form_field fields[]={{"What is your name?",PS_FORM_TEXT,name,sizeof name,1,20,0,0},
        {"What role would you like?",PS_FORM_CHOICE,role,sizeof role,0,0,choices,3}};
    ps_form form={fields,2,0,done,0};ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
#if PS_FORM
    assert(ps_form_open(s,&form));assert(!ps_form_open(s,&form));
    keys(s,"\r");assert(ps_form_active(s)==1&&strstr(output,"required")&&!name[0]);
    keys(s,"abcdefghijklmnopqrstu\r");assert(ps_form_active(s)==1&&!name[0]);
    keys(s,"\025Ada\r");assert(ps_form_active(s)==2&&!strcmp(name,"Ada"));
    keys(s,"\033[B\r");assert(!ps_form_active(s)&&completed==1&&!strcmp(role,"Contributor"));
    assert(ps_form_open(s,&form));keys(s,"\025Discard");ps_key(s,27);
    assert(cancelled==1&&!strcmp(name,"Ada")&&!ps_form_active(s));
#if PS_MOUSE
    assert(ps_form_open(s,&form));keys(s,"\r\033[<0;5;9M\r");
    assert(completed==2&&!strcmp(role,"Contributor"));
    assert(ps_form_open(s,&form));keys(s,"\025abc\033[<0;4;3MZ\r");
    assert(!strcmp(name,"Zabc"));ps_form_close(s);
#endif
    assert(ps_form_open(s,&form));keys(s,"\025A\033[200~B\rC\033[201~");
    assert(ps_form_active(s)==1);keys(s,"\r");assert(!strcmp(name,"ABC"));
    ps_form_close(s);
    cfg.flags=0;s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);used=0;
    assert(!ps_form_open(s,&form)&&!used);cfg.flags=PS_INTERACTIVE;
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(ps_form_open(s,&form));
    keys(s,"\025plain");{unsigned i;for(i=0;i<used;++i)assert((unsigned char)output[i]<128);}
    ps_resize(s,20,8);keys(s,"\033[F");ps_resize(s,96,24);ps_form_close(s);
#else
    (void)keys;(void)validate;
    assert(!ps_form_open(s,&form)&&!ps_form_active(s));ps_form_close(s);
#endif
    if(argc==1)return 0;
    if(argc!=4)return 2;
    cfg.flags=PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE;
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    name[0]=0;fields[0].label="What is your favourite programming language?";
    fields[0].maximum=0;form.count=1;form.validate=validate;ps_replay_reset();
    ps_form_open(s,&form);keys(s,"This answer is longer than twenty characters\r");
    if(save(argv[1]))return 3;
    ps_form_close(s);
    form.fields=fields+1;form.count=1;form.validate=0;ps_replay_reset();
    ps_form_open(s,&form);keys(s,"\033[H");if(save(argv[2]))return 4;
    ps_form_close(s);
    fields[0].label="What is your name?";form.fields=fields;form.count=2;name[0]=0;strcpy(role,"Member");
    ps_replay_reset();ps_form_open(s,&form);keys(s,"Jess Archer\r\033[B");
    return save(argv[3]);
}
