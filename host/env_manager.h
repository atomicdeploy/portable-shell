#ifndef PS_HOST_ENV_MANAGER_H
#define PS_HOST_ENV_MANAGER_H
#include "portable_shell.h"
#include "env_file.h"
#ifndef PS_ENV_VIEW_ROWS
#define PS_ENV_VIEW_ROWS 128u
#endif
#ifndef PS_ENV_LABEL_BYTES
#define PS_ENV_LABEL_BYTES 128u
#endif
#ifndef PS_ENV_DRAFT_BYTES
#define PS_ENV_DRAFT_BYTES 2048u
#endif
#if PS_ENV_VIEW_ROWS < 1 || PS_ENV_VIEW_ROWS > 128
#error PS_ENV_VIEW_ROWS must fit the shared panel capacity of 1 through 128 rows
#endif
typedef int (*env_save_fn)(void *,env_document *,const char **);
typedef struct {
    env_document document;
    ps_session *session;
    ps_panel panel;
    ps_panel_chrome chrome;
    ps_control_page pages[2];
    ps_control controls[2][PS_ENV_VIEW_ROWS];
    char labels[2][PS_ENV_VIEW_ROWS][PS_ENV_LABEL_BYTES];
    char line_numbers[2][PS_ENV_VIEW_ROWS][12];
    char path[PS_ENV_PATH],status[256],notice[128],filter[64];
    char drafts[2][PS_ENV_DRAFT_BYTES];
    ps_form_field fields[2];ps_form form;
    unsigned open,pending,selected,offset,source,returning,form_action,reveal;
    env_save_fn save;void *save_context;
} env_manager;
/* The host owns storage/lifetime. Every input/tick is followed by dispatch. */
int env_manager_open(env_manager *,ps_session *,const char *path,const char *text,unsigned size);
void env_manager_dispatch(env_manager *);
int env_manager_register(ps_session *);
void env_manager_poll(ps_session *);
void env_manager_feed(ps_session *,const void *,unsigned);
#endif
