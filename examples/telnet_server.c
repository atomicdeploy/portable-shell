/* CMake target portable_shell_telnet; Windows, Linux and macOS adapters.
   Run: PortableShellTelnet --listen 2323 --bind 127.0.0.1
   This is the same native adapter, including negotiation, four isolated clients,
   output queues, terminal restoration and serial support; there is no stub. */
#define PS_HOST_EXEC 0 /* This standalone transport example does not launch processes. */
#include "portable_shell.h"
static void consumer_session_ready(ps_session *session);
#define PS_HOST_SESSION_READY(session) consumer_session_ready(session)
#ifdef _WIN32
#define PS_HOSTED_CRT
#include "../host/windows.c"
#else
#include "../host/posix.c"
#endif

static int consumer_demo(ps_session *session, int argc, const char *const *argv, void *context) {
    (void)argc; (void)argv; (void)context;
    return ps_execute(session, "controls");
}
/* Called once per accepted client, before its initial prompt. Register application
   commands here; use command.context for consumer-owned per-session state. */
static void consumer_session_ready(ps_session *session) {
    const ps_command command = {"demo", "Open the editable control panel", "Demo",
                                consumer_demo, 0, 0};
    if (ps_register(session, &command)) {
        ps_write(session, "Command capacity exhausted");
        ps_newline(session);
    }
}
