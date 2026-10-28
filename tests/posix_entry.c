#include "portable_shell.h"
#include <unistd.h>
extern int portable_shell_checks(ps_write_fn report, void *context);
static void report(void *ctx, const ps_byte *p, unsigned n) {
    (void)ctx;
    while (n) {
        ssize_t k = write(1, p, n);
        if (k <= 0)
            return;
        p += k;
        n -= (unsigned)k;
    }
}
int main(void) {
    return portable_shell_checks(report, 0);
}
