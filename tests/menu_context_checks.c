#include "../examples/service_menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
static union {void *align;unsigned char bytes[PS_DEFAULT_SESSION_STORAGE];} memory;
static char output[262144],svg[400000];static unsigned used,actions,completed;
static service_menu_example model;
static char hostname[48]="workbench",password[48]="",access[32]="Operator";
static const char *const roles[]={"Operator","Administrator"};
static ps_form_field fields[]={
    {"Connection name",PS_FORM_TEXT,hostname,sizeof hostname,1,32,0,0},
    {"Access password",PS_FORM_PASSWORD,password,sizeof password,4,32,0,0},
    {"Access level",PS_FORM_CHOICE,access,sizeof access,0,0,roles,PS_COUNT_OF(roles)}
};
static void done(ps_session *s,void *ctx,int accepted){(void)s;(void)ctx;completed+=(unsigned)accepted;}
static ps_form form={fields,PS_COUNT_OF(fields),0,done,0};
static void sink(void *ctx,const ps_byte *p,unsigned n){
    (void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;ps_replay_feed(p,n);
}
static void keys(ps_session *s,const char *text){used=0;ps_feed(s,text,(unsigned)strlen(text));}
static int dispatch(ps_session *s,void *ctx,unsigned action){
    (void)ctx;++actions;
    if(action==SERVICE_SETUP)return ps_form_open(s,&form)?0:1;
    model.notice=action==SERVICE_START?"Simulation: service running":"Simulation: request recorded";
    model.notice_role=PS_INK_SUCCESS;
    return service_menu_open(s,&model)?0:1;
}
static int save(const char *path){unsigned n=ps_replay_svg(svg);FILE *f=fopen(path,"wb");int bad;if(!f)return 1;bad=fwrite(svg,1,n,f)!=n;return fclose(f)||bad;}
int main(int argc,char **argv){
    ps_config cfg={sink,0,96,24,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);unsigned i;
    assert(s);service_menu_init(&model,dispatch,0);
    model.notice="Simulation: service stopped | changes remain local";model.notice_role=PS_INK_WARNING;
    ps_replay_reset();assert(service_menu_open(s,&model));
#if PS_MENU_CONTEXT
    assert(strstr(output,model.notice));
#endif
    if(argc==4&&save(argv[1]))return 2;
    keys(s,"j\r");assert(ps_menu_active(s)==2);
    assert(strstr(output,"Restart service")); /* Marker cells must not clip labels. */
#if PS_MENU_CONTEXT
    assert(strstr(output,"Service console")&&strstr(output,"Service management"));
#endif
    if(argc==4&&save(argv[2]))return 3;
    keys(s,"\033");ps_tick(s,100); /* Resolve an isolated Escape. */
    if(ps_menu_active(s)==2)ps_key(s,27);
    assert(ps_menu_active(s)==1);
#if PS_MENU_CONTEXT
    keys(s,"\r");assert(ps_menu_active(s)==2); /* Return restores parent selection. */
    keys(s,"\r");assert(actions==1&&ps_menu_active(s)==1); /* Start callback. */
    assert(strstr(output,"service running"));
#endif
    ps_menu_close(s);
    {ps_menu_item many[32];ps_menu page={"Paged",many,PS_COUNT_OF(many)};
        for(i=0;i<PS_COUNT_OF(many);++i){ps_menu_item item={"Operation",0,service_menu_dispatch,&model.actions[SERVICE_STATUS],0};many[i]=item;}
        ps_menu_set_style(s,PS_MENU_TABLE|PS_MENU_MARKER);assert(ps_menu_open(s,&page));
        ps_key(s,PS_KEY_PAGEDOWN);ps_key(s,PS_KEY_PAGEUP);ps_key(s,PS_KEY_END);ps_key(s,PS_KEY_HOME);
        many[0].flags=PS_MENU_DISABLED;ps_menu_refresh(s);{unsigned before=actions;ps_key(s,13);assert(actions==before&&ps_menu_active(s));}
        ps_resize(s,20,8);ps_key(s,PS_KEY_PAGEDOWN);ps_menu_close(s);ps_resize(s,96,24);
    }
#if PS_FORM
    ps_replay_reset();used=0;assert(ps_form_open(s,&form));keys(s,"\rnot-a-real-secret");
    assert(!strstr(output,"not-a-real-secret")&&(strstr(output,"•")||strstr(output,"*")));
    if(argc==4&&save(argv[3]))return 4;
    keys(s,"\r");assert(!strcmp(password,"not-a-real-secret"));
    assert(!strstr(output,"not-a-real-secret")); /* Previous accepted answer stays masked. */
    keys(s,"\r");assert(completed==1&&!ps_form_active(s));
    cfg.flags=PS_INTERACTIVE;s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(ps_form_open(s,&form));
    keys(s,"\r\025abc\r");assert(ps_form_active(s)==2&&!strstr(output,"abc")&&strstr(output,"***"));
    keys(s,"\025界ab"); /* A wide code point occupies one mask cell. */
    ps_key(s,PS_KEY_LEFT);ps_key(s,127);ps_key(s,27);assert(!ps_form_active(s));
#endif
    return 0;
}
