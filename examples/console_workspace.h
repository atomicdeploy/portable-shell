#ifndef PORTABLE_SHELL_CONSOLE_WORKSPACE_EXAMPLE_H
#define PORTABLE_SHELL_CONSOLE_WORKSPACE_EXAMPLE_H
/* Hosted, caller-owned example controller. Domain descriptions and provider
   actions are supplied by the consumer, never compiled into PortableShell. */
#include "portable_shell.h"
enum { CW_MENU, CW_TABLE, CW_FORM, CW_DOCUMENT };
enum { CW_TEXT, CW_SECRET, CW_CHOICE, CW_TOGGLE, CW_NUMBER, CW_LABEL, CW_ACTION };
enum { CW_MAX_SCREENS=128, CW_MAX_ENTRIES=80, CW_MAX_VALUES=1024,
       CW_VALUE_SIZE=64, CW_MAX_FIELDS=16, CW_MAX_TABS=8 };
typedef struct {
    const char *label,*initial,*group,*target,*binding;
    unsigned kind,minimum,maximum;
    const char *const *choices; unsigned choice_count;
} cw_entry;
typedef struct {
    const char *id,*title,*notice,*parent,*family,*tab;
    unsigned kind;
    const cw_entry *entries; unsigned count;
    const char *document;
} cw_screen;
typedef struct cw_workspace cw_workspace;
typedef struct {cw_workspace *owner;unsigned entry;} cw_action;
struct cw_workspace {
    ps_session *session;
    const cw_screen *screens;unsigned count,current,revision,requests,accepted;
    unsigned offsets[CW_MAX_SCREENS],slots[CW_MAX_VALUES],value_count;
    char values[CW_MAX_VALUES][CW_VALUE_SIZE];
    ps_menu menu;ps_menu_item menu_items[CW_MAX_ENTRIES];cw_action actions[CW_MAX_ENTRIES];
    ps_panel panel;ps_control_page pages[CW_MAX_TABS];
    ps_control controls[CW_MAX_TABS][CW_MAX_ENTRIES];
    ps_form form;ps_form_field fields[CW_MAX_FIELDS];
    char drafts[CW_MAX_FIELDS][CW_VALUE_SIZE];
    char notice[192];
    const char *pending;
};
int cw_init(cw_workspace *,ps_session *,const cw_screen *,unsigned);
int cw_open(cw_workspace *,const char *id);
void cw_key(cw_workspace *,unsigned);
void cw_feed(cw_workspace *,const char *,unsigned);
void cw_tick(cw_workspace *,ps_u32 milliseconds);
/* Call on the owner loop after direct ps_feed/ps_key, never in provider callbacks. */
void cw_dispatch(cw_workspace *);
#endif
