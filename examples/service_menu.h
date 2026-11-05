#ifndef PORTABLE_SHELL_SERVICE_MENU_EXAMPLE_H
#define PORTABLE_SHELL_SERVICE_MENU_EXAMPLE_H
/* Application-owned example. No service names, backend calls or commands are
   registered by the framework. Retain this model while a menu is open. */
#include "portable_shell.h"
#include "portable_style.h"
enum { SERVICE_SETUP, SERVICE_SETTINGS, SERVICE_STATUS, SERVICE_LOGS,
       SERVICE_START, SERVICE_STOP, SERVICE_RESTART, SERVICE_HEALTH,
       SERVICE_NETWORK, SERVICE_STORAGE, SERVICE_ACTION_COUNT };
typedef int (*service_dispatch_fn)(ps_session *,void *,unsigned action);
typedef struct service_menu_example service_menu_example;
typedef struct {service_menu_example *owner;unsigned id;} service_menu_action;
struct service_menu_example {
    ps_menu root,operations,diagnostics;
    ps_menu_item root_items[6],operation_items[3],diagnostic_items[3];
    service_menu_action actions[SERVICE_ACTION_COUNT];
    service_dispatch_fn dispatch;void *context;
    const char *notice;unsigned notice_role;
};
static int service_menu_dispatch(ps_session *s,void *context){
    service_menu_action *action=(service_menu_action *)context;
    return action->owner->dispatch?action->owner->dispatch(s,action->owner->context,action->id):0;
}
static const char *service_menu_notice(void *context,const ps_menu *menu,unsigned *role){
    service_menu_example *model=(service_menu_example *)context;
    (void)menu;*role=model->notice_role;return model->notice;
}
static const char *service_menu_description(void *context,const ps_menu *menu,unsigned index){
    service_menu_example *model=(service_menu_example *)context;
    static const char *const root[]={"Configure a new connection.","Inspect and change service state.",
        "Run checks through your provider.","Edit typed settings inline.","Inspect current observations.","Review provider events."};
    static const char *const operations[]={"Request a service start.","Request a service stop.","Request a restart after confirmation."};
    static const char *const diagnostics[]={"Inspect provider health.","Check connection availability.","Inspect storage capacity."};
    if(index>=menu->count)return "";
    return menu==&model->root?root[index]:menu==&model->operations?operations[index]:diagnostics[index];
}
static void service_menu_init(service_menu_example *model,service_dispatch_fn dispatch,void *context){
    unsigned i;
    static const char *const labels[]={"Quick setup","Settings","System status","Event log",
        "Start service","Stop service","Restart service","Health check","Network check","Storage check"};
    const unsigned root_actions[]={SERVICE_SETUP,SERVICE_SETTINGS,SERVICE_STATUS,SERVICE_LOGS};
    const unsigned root_slots[]={0,3,4,5};
    model->dispatch=dispatch;model->context=context;model->notice="Provider not connected";model->notice_role=PS_INK_INACTIVE;
    for(i=0;i<SERVICE_ACTION_COUNT;++i){model->actions[i].owner=model;model->actions[i].id=i;}
    for(i=0;i<PS_COUNT_OF(root_actions);++i){unsigned id=root_actions[i];
        ps_menu_item item={labels[id],0,service_menu_dispatch,&model->actions[id],0};model->root_items[root_slots[i]]=item;}
    for(i=0;i<PS_COUNT_OF(model->operation_items);++i){unsigned id=SERVICE_START+i;
        ps_menu_item item={labels[id],0,service_menu_dispatch,&model->actions[id],0};model->operation_items[i]=item;}
    for(i=0;i<PS_COUNT_OF(model->diagnostic_items);++i){unsigned id=SERVICE_HEALTH+i;
        ps_menu_item item={labels[id],0,service_menu_dispatch,&model->actions[id],0};model->diagnostic_items[i]=item;}
    model->operations.title="Service management";model->operations.items=model->operation_items;model->operations.count=PS_COUNT_OF(model->operation_items);
    model->diagnostics.title="Diagnostics";model->diagnostics.items=model->diagnostic_items;model->diagnostics.count=PS_COUNT_OF(model->diagnostic_items);
    {ps_menu_item item={"Service management",&model->operations,0,0,0};model->root_items[1]=item;}
    {ps_menu_item item={"Diagnostics",&model->diagnostics,0,0,0};model->root_items[2]=item;}
    model->root.title="Service console";model->root.items=model->root_items;model->root.count=PS_COUNT_OF(model->root_items);
}
static int service_menu_open(ps_session *s,service_menu_example *model){
    ps_menu_set_style(s,PS_MENU_TABLE|PS_MENU_DETAILS|PS_MENU_BREADCRUMBS|PS_MENU_MARKER);
    ps_menu_set_description(s,service_menu_description,model);ps_menu_set_notice(s,service_menu_notice,model);
    return ps_menu_open(s,&model->root);
}
#endif
