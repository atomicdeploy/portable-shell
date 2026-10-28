#include "portable_shell.h"
#include <stdio.h>
extern int portable_shell_checks(ps_write_fn, void *);
static void report(void *context, const ps_byte *bytes, unsigned count) {
    (void)context;
    (void)fwrite(bytes, 1, count, stdout);
}
int main(void) {
    return portable_shell_checks(report, 0);
}
