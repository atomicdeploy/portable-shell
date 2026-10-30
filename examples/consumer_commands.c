/* Hosted stdin/stdout transport example. The application supplies every command,
 * description and setting; the linked framework installs none. */
#include "portable_settings.h"
#include <stdio.h>
static union {void *alignment;unsigned char bytes[65536];} storage;
static unsigned brightness=50;
static int read_level(void *ctx,ps_setting_value *v){v->natural=*(unsigned *)ctx;return 1;}
static int write_level(void *ctx,const ps_setting_value *v){*(unsigned *)ctx=v->natural;return 1;}
static int help(ps_session *s,int argc,const char *const *argv,void *ctx){
    (void)ctx;return ps_print_help(s,argc==2?argv[1]:0,PS_HELP_HEADINGS)?0:1;
}
static void output(void *ctx,const ps_byte *p,unsigned n){(void)ctx;fwrite(p,1,n,stdout);}
int main(int argc,char **argv){
    ps_settings settings;const ps_setting *slots[8];
    ps_setting level={0};ps_config config={output,0,80,24,0};
    ps_session *session=ps_init(storage.bytes,sizeof storage.bytes,&config);
    const ps_help_item examples[]={{"config brightness 75","Adjust the output level."}};
    const ps_help_document documentation={"config","Inspect or change application settings.",0,0,0,0,examples,1};
    ps_command_definition commands[]={
        {{"config","[KEY [VALUE]]","Device",ps_settings_command,0,&settings},&documentation},
        {{"help","[COMMAND]","Reference",help,0,0},0}
    };
    unsigned i;int status=0,c;
    if(!session)return 1;
    ps_set_application_title(session,"Device console");
    ps_set_prompt(session,"device> ",PS_STYLE_PROMPT);
    ps_settings_init(&settings,slots,8);
    level.name="brightness";level.description="Output level";level.group="Lighting";
    level.type=PS_SETTING_UINT;level.unsigned_maximum=100;level.read=read_level;level.write=write_level;level.context=&brightness;
    if(ps_settings_register(&settings,&level))return 1;
    for(i=0;i<sizeof commands/sizeof commands[0];++i)if(ps_register_definition(session,commands+i))return 1;
    if(argc>1)status=ps_execute_argv(session,argc-1,(const char *const *)(argv+1));
    else {while((c=getchar())!=EOF){ps_byte b=(ps_byte)c;ps_feed(session,&b,1);}ps_finish(session);status=ps_status(session);}
    ps_destroy_session(session);return status;
}
