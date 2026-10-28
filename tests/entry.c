#include "portable_shell.h"
/* Import-free test executable. The native loader invokes this entry point.
   No stdin, stdout, CRT startup, or OS API is linked. Debuggers can inspect
   test_report at test_complete, and the loader observes the return status. */
char test_report[4096];
unsigned test_report_length;
extern int portable_shell_checks(ps_write_fn report, void *context);
static void report(void *context, const ps_byte *bytes, unsigned count) {
    (void)context;
    while (count-- && test_report_length + 1 < sizeof test_report)
        test_report[test_report_length++] = (char)*bytes++;
    test_report[test_report_length] = 0;
}
__attribute__((noinline)) int test_complete(int failures) {
    return failures;
}
int test_entry(void) {
    return test_complete(portable_shell_checks(report, 0));
}
