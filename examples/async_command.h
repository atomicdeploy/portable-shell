#ifndef PORTABLE_ASYNC_COMMAND_EXAMPLE_H
#define PORTABLE_ASYNC_COMMAND_EXAMPLE_H
/* Embed this context alongside a session; drive tick from the owning event loop.
 * A real adapter replaces step with bounded work, or submits with NULL step and
 * calls ps_async_complete when a device/socket/worker event arrives. */
#include "portable_shell.h"
#include "portable_async.h"
typedef struct {
    ps_session *shell;
    ps_async pool;
    ps_async_job slot;
    uint32_t now, id;
    unsigned progress;
    char text[PS_LINE];
} async_command_example;
static unsigned example_step(void *context,uint32_t now,int *result) {
    async_command_example *app=(async_command_example *)context;
    (void)now;
    if(++app->progress<3)return PS_ASYNC_PENDING;
    *result=0;return PS_ASYNC_DONE;
}
static void example_done(void *context,uint32_t id,unsigned state,int result) {
    async_command_example *app=(async_command_example *)context;
    (void)id;(void)result;
    app->id=0;
    ps_message(app->shell,state==PS_ASYNC_DONE?PS_STYLE_SUCCESS:PS_STYLE_WARNING,
               state==PS_ASYNC_DONE?app->text:"Deferred command cancelled or timed out");
}
static int example_command(ps_session *shell,int argc,const char *const *argv,void *context) {
    async_command_example *app=(async_command_example *)context;
    unsigned n=0;
    (void)shell;
    if(argc!=2 || app->id)return 1;
    while(argv[1][n] && n+1<sizeof app->text){app->text[n]=argv[1][n];++n;}
    if(argv[1][n])return 1;
    app->text[n]=0;app->progress=0;
    app->id=ps_async_submit(&app->pool,app->now,5000,example_step,0,example_done,app);
    /* This is acceptance status, not eventual operation status. */
    return app->id?0:1;
}
static int example_async_init(async_command_example *app,ps_session *shell) {
    ps_command command={"later","later <text>","Async",example_command,0,app};
    app->shell=shell;app->now=app->id=0;app->progress=0;app->text[0]=0;
    return ps_async_init(&app->pool,&app->slot,1) && ps_register(shell,&command)==0;
}
static void example_async_tick(async_command_example *app,uint32_t now) {
    app->now=now;
    ps_async_poll(&app->pool,now,1);
    ps_tick(app->shell,now);
}
#endif
