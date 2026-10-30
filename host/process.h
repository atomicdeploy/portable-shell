#ifndef PS_HOST_PROCESS_H
#define PS_HOST_PROCESS_H
#ifndef PS_HOST_EXEC
#define PS_HOST_EXEC 1
#endif
/* Host-only. argv is passed literally; use an explicit sh/cmd invocation for pipelines.
   PTYs merge stdout/stderr by terminal design; normal execution preserves both. */
int ps_host_process(int argc, const char *const *argv, int terminal);
#endif
