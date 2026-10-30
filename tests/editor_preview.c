/* Host-only replay fixture: all UI bytes come from the real core and keystrokes. */
#include <stdio.h>
#include <string.h>
#include "portable_shell.h"
static union { void *align; unsigned char bytes[40000]; } memory;
static void write_bytes(void *ctx, const ps_byte *bytes, unsigned count) {
    fwrite(bytes, 1, count, (FILE *)ctx);
}
int main(int argc, char **argv) {
    FILE *out;
    ps_config config;
    ps_session *s;
    const char *keys;
    if (argc != 3) return 2;
    out = fopen(argv[1], "wb");
    if (!out) return 3;
    config.write = write_bytes; config.context = out;
    config.columns = 96; config.rows = 36;
    config.flags = PS_INTERACTIVE | PS_COLOR | PS_TRUECOLOR | PS_UNICODE;
    s = ps_init(memory.bytes, sizeof memory.bytes, &config);
    if (!s) { fclose(out); return 4; }
    ps_execute(s, !strcmp(argv[2], "cyan") ? "theme cyan" : "theme onedark");
    ps_start(s);
    keys = "help\rtheme cya";
    ps_feed(s, keys, (unsigned)strlen(keys));
    return fclose(out) != 0;
}
