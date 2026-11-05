/* Optional Windows/POSIX native demo; reuses the existing console/Telnet host. */
#define PS_HOST_EXEC 0
#include "portable_shell.h"
static void settings_session_ready(ps_session *);
#define PS_HOST_SESSION_READY(session) settings_session_ready(session)
#ifdef _WIN32
#define PS_HOSTED_CRT
#include "../host/windows.c"
#else
#include "../host/posix.c"
#endif
#include "settings_editor_model.h"
static settings_demo models[CLIENTS];
static void settings_session_ready(ps_session *session){
    unsigned i;
    /* Native adapters use stable session slots, reused on reconnect. */
    for(i=0;i<CLIENTS;++i)if(sessions[i]==session)break;
    /* The local console occupies slot zero but does not populate sessions[]. */
    if(i==CLIENTS)i=0;
    if(!settings_demo_init(models+i,session))ps_message(session,PS_STYLE_ERROR,"Settings editor unavailable");
}
