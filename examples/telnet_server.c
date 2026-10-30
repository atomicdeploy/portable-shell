/* Compile this example with the core or use the CMake PortableShell target.
   Run: PortableShell --listen 2323
   This is the same native adapter, including negotiation, four isolated clients,
   output queues, terminal restoration and serial support; there is no stub. */
#define PS_HOST_EXEC 0 /* This standalone transport example does not launch processes. */
#ifdef _WIN32
#define PS_HOSTED_CRT
#include "../host/windows.c"
#else
#include "../host/posix.c"
#endif
