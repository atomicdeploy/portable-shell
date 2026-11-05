/* Deterministic interaction fixture; every displayed byte comes from the core. */
#include "../examples/service_menu.h"
#include "../tools/terminal_recording.h"
#include <string.h>
enum { DEMO_COLUMNS=96, DEMO_ROWS=24, KEY_DELAY_MS=110 };
static union {void *align;unsigned char bytes[PS_DEFAULT_SESSION_STORAGE];} storage;
static FILE *recording;
static unsigned now;
static int failed;
static service_menu_example model;
static char name[48]="",access[32]="Operator";
static const char *const roles[]={"Operator","Administrator"};
static ps_form_field fields[]={
    {"Connection name",PS_FORM_TEXT,name,sizeof name,1,32,0,0},
    {"Access level",PS_FORM_CHOICE,access,sizeof access,0,0,roles,PS_COUNT_OF(roles)}
};
static void complete(ps_session *s,void *context,int accepted){
    (void)context;
    model.notice=accepted?"Saved locally: connection ready":"Setup cancelled";
    model.notice_role=accepted?PS_INK_SUCCESS:PS_INK_WARNING;
    service_menu_open(s,&model);
}
static ps_form form={fields,PS_COUNT_OF(fields),0,complete,0};
static int dispatch(ps_session *s,void *context,unsigned action){
    (void)context;
    if(action==SERVICE_SETUP)return ps_form_open(s,&form)?0:1;
    model.notice=action==SERVICE_START?"Simulation: service running":"Simulation: health check passed";
    model.notice_role=PS_INK_SUCCESS;return service_menu_open(s,&model)?0:1;
}
static int command(ps_session *s,int argc,const char *const *argv,void *context){
    (void)argc;(void)argv;(void)context;return service_menu_open(s,&model)?0:1;
}
static void output(void *context,const ps_byte *bytes,unsigned count){
    (void)context;if(!ps_record_event(recording,now,'O',bytes,count))failed=1;
}
static void frame(void){if(!ps_record_event(recording,now,'F',0,0))failed=1;}
static void keys(ps_session *s,unsigned delay,const char *bytes){
    now+=delay;
    if(!ps_record_event(recording,now,'I',bytes,(unsigned)strlen(bytes)))failed=1;
    ps_feed(s,bytes,(unsigned)strlen(bytes));ps_tick(s,now);frame();
}
static void typing(ps_session *s,const char *text){
    while(*text){char byte[2]={*text++,0};keys(s,KEY_DELAY_MS,byte);}
}
int main(int argc,char **argv){
    ps_config config={output,0,DEMO_COLUMNS,DEMO_ROWS,PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_TRUECOLOR};
    ps_command menu={"settings","Open the service workspace","Demo",command,0,0};
    ps_session *s;
    if(argc!=2)return 2;
    recording=fopen(argv[1],"wb");if(!recording)return 3;
    if(!ps_record_begin(recording,DEMO_COLUMNS,DEMO_ROWS))failed=1;
    s=ps_init(storage.bytes,sizeof storage.bytes,&config);if(!s){fclose(recording);return 4;}
    service_menu_init(&model,dispatch,0);
    model.notice="Simulation: service stopped | changes remain local";model.notice_role=PS_INK_WARNING;
    if(ps_register(s,&menu)){fclose(recording);return 5;}
    ps_set_prompt(s,"╭─ operator@workbench\n╰─❯ ",PS_STYLE_PROMPT);ps_start(s);frame();
    now+=1200;typing(s,"settings");keys(s,900,"\r");
    keys(s,1200,"j");keys(s,650,"\r");keys(s,1400,"j");keys(s,650,"k");keys(s,650,"\r");
    keys(s,1400,"\r"); /* Root selection resets to setup after the callback. */
    keys(s,1000,"\r"); /* Empty required text: show validation. */
    typing(s,"workbench");keys(s,1200,"\r");keys(s,900,"\033[B");keys(s,1000,"\r");
    if(strcmp(name,"workbench")||strcmp(access,"Administrator")||ps_form_active(s))failed=1;
    now+=500;frame(); /* Identical frame: tests idle checkpoint coalescing. */
    now+=2500;if(!ps_record_event(recording,now,'E',0,0))failed=1;
    if(fclose(recording))failed=1;
    return failed?6:0;
}
