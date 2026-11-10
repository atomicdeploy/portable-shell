#ifndef PS_SETTINGS_EDITOR_MODEL_H
#define PS_SETTINGS_EDITOR_MODEL_H
#include "portable_settings_editor.h"
/* Synthetic, per-session application settings. Replace these callbacks with
   your own observations and writes; persistence is deliberately consumer-owned. */
enum { SETTINGS_DEMO_COUNT=14 };
typedef struct {ps_setting descriptor;ps_setting_value value;char text[64];} settings_demo_item;
typedef struct {
    ps_settings registry;const ps_setting *slots[SETTINGS_DEMO_COUNT];
    settings_demo_item items[SETTINGS_DEMO_COUNT];
    ps_settings_editor editor;ps_control controls[SETTINGS_DEMO_COUNT];
    ps_settings_editor_row rows[SETTINGS_DEMO_COUNT];
    unsigned key_names;
} settings_demo;
static int settings_demo_key(ps_session *session,unsigned key,void *context){
    settings_demo *model=(settings_demo *)context;
    if(key!=PS_KEY_F6)return 0;
    if(ps_panel_set_key_names(session,!model->key_names))model->key_names=!model->key_names;
    return 1;
}
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
    if(!ps_settings_editor_open(&model->editor,s))return 1;
    model->key_names=0;ps_panel_set_key_handler(s,settings_demo_key,model);return 0;
}
static int settings_demo_init(settings_demo *model,ps_session *session){
    static const char *const modes[]={"Development","Production","Maintenance"};
    static const char *const themes[]={"system","dark","light"};
    static const char *const palettes[]={"default","cyan","violet","red","one-dark"};
    static const char *const densities[]={"comfortable","compact","dense"};
    static const char *const cursors[]={"block","underline","bar"};
    static const char *const theme_labels[]={"Follow application","Dark","Light"};
    static const char *const palette_labels[]={"Default","Cyan","Violet","Red","One Dark"};
    static const char *const density_labels[]={"Comfortable","Compact","Dense"};
    static const char *const cursor_labels[]={"Block","Underline","Vertical bar"};
    static const ps_setting_presentation presentation[]={
        {"api.base_url","Base URL",0,0},
        {"terminal.theme","Theme",theme_labels,PS_COUNT_OF(theme_labels)},
        {"terminal.palette","Palette",palette_labels,PS_COUNT_OF(palette_labels)},
        {"terminal.density","Interface density",density_labels,PS_COUNT_OF(density_labels)},
        {"terminal.cursor_style","Cursor style",cursor_labels,PS_COUNT_OF(cursor_labels)}
    };
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
        {"runtime.version","1.0",PS_SETTING_TEXT,0,0},
        {"terminal.theme","system",PS_SETTING_CHOICE,0,0},
        {"terminal.palette","default",PS_SETTING_CHOICE,0,0},
        {"terminal.density","comfortable",PS_SETTING_CHOICE,0,0},
        {"terminal.cursor_style","bar",PS_SETTING_CHOICE,0,0}
    };
    unsigned i;
    ps_settings_init(&model->registry,model->slots,PS_COUNT_OF(model->slots));
    for(i=0;i<PS_COUNT_OF(definitions);++i){settings_demo_item *item=model->items+i;ps_setting *d=&item->descriptor;
        d->name=definitions[i].name;d->description="Example configuration value";d->group=0;d->type=definitions[i].type;
        d->minimum=definitions[i].low;d->maximum=definitions[i].high;
        d->unsigned_minimum=(unsigned)definitions[i].low;d->unsigned_maximum=(unsigned)definitions[i].high;
        if(ps_text_equal(d->name,"network.timeout_ms"))d->unsigned_maximum=0xffffffffu;
        d->choices=modes;d->choice_count=PS_COUNT_OF(modes);d->max_length=sizeof item->text-1;
        if(ps_text_equal(d->name,"terminal.theme")){d->choices=themes;d->choice_count=PS_COUNT_OF(themes);}
        if(ps_text_equal(d->name,"terminal.palette")){d->choices=palettes;d->choice_count=PS_COUNT_OF(palettes);}
        if(ps_text_equal(d->name,"terminal.density")){d->choices=densities;d->choice_count=PS_COUNT_OF(densities);}
        if(ps_text_equal(d->name,"terminal.cursor_style")){d->choices=cursors;d->choice_count=PS_COUNT_OF(cursors);}
        d->read=settings_demo_read;d->write=ps_text_equal(d->name,"runtime.version")?0:settings_demo_write;
        d->validate=settings_demo_validate;d->context=item;
        if(ps_setting_parse(d,definitions[i].initial,&item->value))return 0;
        ps_terminal_format(item->text,sizeof item->text,"%s",definitions[i].initial);
        if(ps_settings_register(&model->registry,d))return 0;
    }
    if(!ps_settings_editor_init(&model->editor,&model->registry,model->controls,model->rows,
                               PS_COUNT_OF(model->rows),"Configuration"))return 0;
    if(!ps_settings_editor_set_presentation(&model->editor,presentation,PS_COUNT_OF(presentation)))return 0;
    model->editor.chrome.navigation="↑/↓ rows  Tab pages  F6 labels/keys  Esc shell";
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
