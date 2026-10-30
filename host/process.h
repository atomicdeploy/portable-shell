#ifndef PS_HOST_PROCESS_H
#define PS_HOST_PROCESS_H
#ifndef PS_HOST_LANGUAGE
#define PS_HOST_LANGUAGE 1
#endif
#ifndef PS_HOST_EXEC
#define PS_HOST_EXEC 1
#endif
#include "portable_command_line.h"
/* Host-only. argv is passed literally.
   PTYs merge stdout/stderr by terminal design; normal execution preserves both. */
int ps_host_pipeline(unsigned count, const ps_shell_command *commands);
int ps_host_process(int argc, const char *const *argv, int terminal);
#endif
