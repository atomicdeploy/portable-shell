/* Hosted evidence utility: capture real adapter output with local bound values. */
#include "portable_shell_legacy.h"
#include <stdio.h>
static FILE *capture;
static int write_byte(void *context, char byte) {
    (void)context;
    return fputc((unsigned char)byte, capture) == EOF ? -1 : 0;
}
int main(int argc, char **argv) {
    PSConfig config = {0};
    PortableShell *shell;
    int level = 42;
    float gain = 1.25f;
    bool enabled = true;
    char label[32] = "Local fixture";
    if (argc != 2 || !(capture = fopen(argv[1], "wb"))) return 2;
    config.write_char = write_byte;
    config.vt100_enabled = true;
    config.prompt = "console> ";
    shell = ps_create(&config);
    if (!shell) { fclose(capture); return 3; }
    ps_resize(shell->core, 96, 24);
    if (ps_register_parameter(shell, "level", PS_PARAM_INT, &level, 0) ||
        ps_register_parameter(shell, "gain", PS_PARAM_FLOAT, &gain, 0) ||
        ps_register_parameter(shell, "enabled", PS_PARAM_BOOL, &enabled, 0) ||
        ps_register_parameter(shell, "label", PS_PARAM_STRING, label, sizeof label) ||
        ps_execute(shell->core, "edit")) {
        ps_destroy(shell); fclose(capture); return 5;
    }
    ps_tick(shell->core, 100);
    /* Capture before close restores the main screen. */
    if (fclose(capture)) return 4;
    shell->io_error = 1;
    ps_destroy(shell);
    return 0;
}
