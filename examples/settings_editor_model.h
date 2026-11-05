#ifndef PS_SETTINGS_EDITOR_MODEL_H
#define PS_SETTINGS_EDITOR_MODEL_H
#include "portable_settings_editor.h"
/* Synthetic, per-session application settings. Replace these callbacks with
   your own observations and writes; persistence is deliberately consumer-owned. */
enum { SETTINGS_DEMO_COUNT=10 };
typedef struct {ps_setting descriptor;ps_setting_value value;char text[64];} settings_demo_item;
typedef struct {
    ps_settings registry;const ps_setting *slots[SETTINGS_DEMO_COUNT];
    settings_demo_item items[SETTINGS_DEMO_COUNT];
    ps_settings_editor editor;ps_control controls[SETTINGS_DEMO_COUNT];
    ps_settings_editor_row rows[SETTINGS_DEMO_COUNT];
} settings_demo;
static int settings_demo_read(void *context,ps_setting_value *value){
    settings_demo_item *item=(settings_demo_item *)context;*value=item->value;value->text=item->text;return 1;
}
static int settings_demo_write(void *context,const ps_setting_value *value){
    settings_demo_item *item=(settings_demo_item *)context;
    item->value.integer=value->integer;item->value.natural=value->natural;
    if(item->descriptor.type==PS_SETTING_TEXT)ps_terminal_format(item->text,sizeof item->text,"%s",value->text);
    return 1;
}
static int settings_demo_validate(void *context,const ps_setting_value *value){
    settings_demo_item *item=(settings_demo_item *)context;
    if(ps_text_equal(item->descriptor.name,"api.base_url")){
        const char *p=value->text,*prefix="http://";
        while(*prefix&&*p==*prefix){++p;++prefix;}
        return !*prefix&&*p;
    }
    return 1;
}
static int settings_demo_open(ps_session *s,int argc,const char *const *argv,void *context){
    settings_demo *model=(settings_demo *)context;(void)argv;
    if(argc!=1)return 2;
    ps_set_table_style(s,PS_TABLE_DEFAULT|PS_TABLE_RED_BORDER);
    return ps_settings_editor_open(&model->editor,s)?0:1;
}
static int settings_demo_init(settings_demo *model,ps_session *session){
    static const char *const modes[]={"Development","Production","Maintenance"};
    static const struct {const char *name,*initial;unsigned type;int low,high;} definitions[]={
        {"api.base_url","http://localhost:8000",PS_SETTING_TEXT,0,0},
        {"api.port","8000",PS_SETTING_UINT,1,65535},
        {"api.enabled","true",PS_SETTING_BOOL,0,0},
        {"application.mode","Development",PS_SETTING_CHOICE,0,0},
        {"application.debug","true",PS_SETTING_BOOL,0,0},
        {"application.timezone","UTC",PS_SETTING_TEXT,0,0},
        {"display.brightness","70",PS_SETTING_UINT,0,100},
        {"display.offset","-12",PS_SETTING_INT,-100,100},
        {"network.timeout_ms","120000",PS_SETTING_UINT,0,0},
        {"runtime.version","1.0",PS_SETTING_TEXT,0,0}
    };
    unsigned i;
    ps_settings_init(&model->registry,model->slots,PS_COUNT_OF(model->slots));
    for(i=0;i<PS_COUNT_OF(definitions);++i){settings_demo_item *item=model->items+i;ps_setting *d=&item->descriptor;
        d->name=definitions[i].name;d->description="Example configuration value";d->group=0;d->type=definitions[i].type;
        d->minimum=definitions[i].low;d->maximum=definitions[i].high;
        d->unsigned_minimum=(unsigned)definitions[i].low;d->unsigned_maximum=(unsigned)definitions[i].high;
        if(ps_text_equal(d->name,"network.timeout_ms"))d->unsigned_maximum=0xffffffffu;
        d->choices=modes;d->choice_count=PS_COUNT_OF(modes);d->max_length=sizeof item->text-1;
        d->read=settings_demo_read;d->write=ps_text_equal(d->name,"runtime.version")?0:settings_demo_write;
        d->validate=settings_demo_validate;d->context=item;
        if(ps_setting_parse(d,definitions[i].initial,&item->value))return 0;
        ps_terminal_format(item->text,sizeof item->text,"%s",definitions[i].initial);
        if(ps_settings_register(&model->registry,d))return 0;
    }
    if(!ps_settings_editor_init(&model->editor,&model->registry,model->controls,model->rows,
                               PS_COUNT_OF(model->rows),"Configuration"))return 0;
    model->editor.panel.status="Example values | Changes stay in memory";
    if(session){
        const ps_command commands[]={
            {"configure","Open typed configuration editor","Settings",settings_demo_open,0,model},
            {"config","[KEY [VALUE]]","Settings",ps_settings_command,0,&model->registry}
        };
        for(i=0;i<PS_COUNT_OF(commands);++i)if(ps_register(session,commands+i))return 0;
    }
    return 1;
}
#endif
