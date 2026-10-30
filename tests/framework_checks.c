#include "portable_settings.h"
#include <assert.h>
#include <string.h>
#include <limits.h>
static union {void *align;unsigned char bytes[65536];} storage;
static char output[65536];static unsigned used,writes,state;
static void sink(void *c,const ps_byte *p,unsigned n){(void)c;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;}
static int run(ps_session *s,int n,const char *const *v,void *c){(void)s;(void)v;*(unsigned *)c+=(unsigned)n;return 7;}
static int read_value(void *c,ps_setting_value *v){(void)c;v->natural=state;return 1;}
static int write_value(void *c,const ps_setting_value *v){(void)c;++writes;state=v->natural;return 1;}
static int validate(void *c,const ps_setting_value *v){(void)c;return v->natural!=13;}
int main(void){
 ps_config config={sink,0,80,24,0};ps_session *s=ps_init(storage.bytes,sizeof storage.bytes,&config);
 ps_help_labels labels={"ABOUT","SYNTAX","INPUTS","FLAGS","SAMPLES"};
 ps_help_document doc={"custom","Consumer-owned explanation",0,0,0,0,0,0};
 unsigned calls=0;ps_command_definition definition={{"custom","[VALUE]","Consumer",run,0,&calls},&doc};
 const ps_setting *slots[2];ps_settings r;ps_setting entry={0},other={0},third;ps_setting_value value;
 const char *choices[]={"low","high"};
 assert(s&&ps_command_count(s)==0);
 assert(ps_execute(s,"help")!=0);assert(ps_command_count(s)==0);
 ps_set_help_labels(s,&labels);ps_set_application_title(s,"My appliance");
 doc.name="wrong";assert(ps_register_definition(s,&definition)!=0&&ps_command_count(s)==0);doc.name="custom";
 assert(ps_register_definition(s,&definition)==0);
 assert(ps_execute(s,"custom value")==7&&calls==2);
 used=0;output[0]=0;assert(ps_print_help(s,"custom",PS_HELP_HEADINGS));assert(strstr(output,"SYNTAX"));
#if PS_HELP_DETAILS
 assert(strstr(output,"Consumer-owned explanation")&&strstr(output,"ABOUT"));
#endif
 assert(ps_register(s,&definition.command)==0);
 used=0;output[0]=0;assert(ps_print_help(s,"custom",PS_HELP_HEADINGS));assert(strstr(output,"SYNTAX"));
 assert(!strstr(output,"Consumer-owned explanation"));
 assert(ps_unregister(s,"custom")==0&&ps_command_count(s)==0);
 assert(ps_set_prompt(s,0,0)==0);
 ps_settings_init(&r,slots,2);entry.name="level";entry.type=PS_SETTING_UINT;
 entry.unsigned_maximum=100;entry.read=read_value;entry.write=write_value;entry.validate=validate;
 if(!PS_SETTINGS){assert(ps_settings_register(&r,&entry)==PS_SETTING_INVALID);assert(!ps_settings_find(&r,"level"));ps_destroy_session(s);return 0;}
 assert(!ps_settings_register(&r,&entry));
 assert(!ps_settings_set(&r,"level","75")&&state==75&&writes==1);
 assert(ps_settings_set(&r,"level","101")==PS_SETTING_INVALID&&writes==1);
 assert(ps_settings_set(&r,"level","13")==PS_SETTING_REJECTED&&writes==1);
 assert(ps_settings_set(&r,"missing","1")==PS_SETTING_NOT_FOUND);
 entry.write=0;assert(ps_settings_set(&r,"level","2")==PS_SETTING_READ_ONLY);entry.write=write_value;
 assert(!ps_settings_get(&r,"level",&value)&&value.natural==75);
 entry.unsigned_maximum=0xffffffffu;
 assert(!ps_setting_parse(&entry,"4294967295",&value)&&value.natural==0xffffffffu);
 assert(ps_setting_parse(&entry,"4294967296",&value)==PS_SETTING_INVALID);
 assert(ps_setting_parse(&entry,"-1",&value)==PS_SETTING_INVALID);
 entry.type=PS_SETTING_INT;entry.minimum=INT_MIN;entry.maximum=INT_MAX;
 assert(!ps_setting_parse(&entry,"-2147483648",&value)&&value.integer==INT_MIN);
 assert(ps_setting_parse(&entry,"2147483648",&value)==PS_SETTING_INVALID);
 assert(ps_setting_parse(&entry,"-2147483649",&value)==PS_SETTING_INVALID);
 assert(ps_setting_parse(&entry,"1x",&value)==PS_SETTING_INVALID);
 entry.type=PS_SETTING_BOOL;assert(!ps_setting_parse(&entry,"true",&value)&&value.natural==1);
 assert(!ps_setting_parse(&entry,"false",&value)&&value.natural==0);
 assert(ps_setting_parse(&entry,"2",&value)==PS_SETTING_INVALID);
 entry.type=PS_SETTING_CHOICE;entry.choices=choices;entry.choice_count=2;
 assert(!ps_setting_parse(&entry,"high",&value)&&value.natural==1);
 assert(ps_setting_parse(&entry,"other",&value)==PS_SETTING_INVALID);
 entry.type=PS_SETTING_TEXT;entry.max_length=3;
 assert(!ps_setting_parse(&entry,"abc",&value));
 assert(ps_setting_parse(&entry,"abcd",&value)==PS_SETTING_INVALID);
 other=entry;other.name="other";assert(!ps_settings_register(&r,&other));
 assert(!ps_settings_register(&r,&entry)&&r.count==2);
 third=entry;third.name="third";assert(ps_settings_register(&r,&third)==PS_SETTING_FULL);
 assert(!ps_settings_unregister(&r,"level")&&r.count==1&&r.entries[0]==&other);
 ps_destroy_session(s);return 0;
}
