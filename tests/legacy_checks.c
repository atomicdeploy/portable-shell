#include "portable_shell_legacy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static char output[131072];
static unsigned used, called;
static int reads;
static int input(void *ctx) {
    (void)ctx;
    ++reads;
    return -1;
}
static int output_string(void *ctx, const char *s) {
    size_t n = strlen(s);
    (void)ctx;
    if (used + n >= sizeof(output))
        return -1;
    memcpy(output + used, s, n + 1);
    used += (unsigned)n;
    return 0;
}
static void clear(void) {
    used = 0;
    output[0] = 0;
}
static int command(PortableShell *s, int argc, char **argv) {
    (void)s;
    ++called;
    assert(argc == 2 && !strcmp(argv[1], "a b") && argv[argc] == NULL);
    argv[1][0] = 'z';
    return 0;
}
static void type(PortableShell *s, const char *text) {
    while (*text)
        ps_process_char(s, *text++);
}
int main(void) {
    PSConfig c = {0};
    PortableShell *s, *other;
    int integer = -5, second = 7;
    float real = 1.25f;
    bool enabled = false;
    char label[32] = "hello", maximum[64];
    ps_control_value observed, requested;
    c.read_char = input;
    c.write_str = output_string;
    c.history_size = 2;
    c.prompt = "legacy> ";
    s = ps_create(&c);
    other = ps_create(&c);
    assert(s && other);
    clear();
    ps_print_prompt(s);
    ps_print_prompt(s);
    assert(!strcmp(output, "legacy> legacy> "));
    clear();
    ps_run(s);
    ps_run(s);
    assert(!strcmp(output, "legacy> legacy> "));
    reads = 0;
    assert(!ps_unregister_command(s, "help"));
    assert(!ps_unregister_command(s, "clear"));
    assert(!ps_register_command(s, "help", "", ps_cmd_help));
    assert(!ps_register_command(s, "clear", "", ps_cmd_clear));
    clear();
    assert(!ps_execute(s->core, "help"));
    assert(strstr(output, "help ") && strstr(output, "clear ") && strstr(output, "theme "));
    assert(!ps_command_count(NULL) && !ps_command_at(NULL, 0));
    assert(!ps_command_at(s->core, ps_command_count(s->core)));
    assert(ps_register_command(s, "custom", "VALUE", command) == 0);
    assert(ps_execute(s->core, "custom 'a b'") == 0 && called == 1);
    assert(ps_unregister_command(s, "custom") == 0);
    assert(ps_execute(s->core, "custom 'a b'") != 0 && called == 1);
    assert(!ps_register_parameter(s, "integer", PS_PARAM_INT, &integer, 0));
    assert(!ps_register_parameter(s, "integer", PS_PARAM_INT, &integer, 0));
    assert(ps_register_parameter(s, "integer", PS_PARAM_INT, &second, 0) < 0);
    assert(!ps_register_parameter(s, "real", PS_PARAM_FLOAT, &real, 0));
    assert(!ps_register_parameter(s, "enabled", PS_PARAM_BOOL, &enabled, 0));
    assert(!ps_register_parameter(s, "label", PS_PARAM_STRING, label, sizeof(label)));
    type(s, "set integer -123\nset real 2.75\nset enabled ON\nset label 'quoted value'\n");
    assert(integer == -123 && real == 2.75f && enabled && !strcmp(label, "quoted value"));
    assert(!ps_execute(s->core, "set enabled NO") && !enabled);
    assert(!ps_execute(s->core, "set enabled YeS") && enabled);
    assert(!strcmp(s->controls[0].group, "string"));
    assert(!strcmp(s->controls[1].group, "bool"));
    assert(!strcmp(s->controls[2].group, "float"));
    assert(!strcmp(s->controls[3].group, "int"));
    type(s, "set integer 123junk\nset real nan\nset enabled maybe\n");
    assert(integer == -123 && real == 2.75f && enabled);
    snprintf(maximum, sizeof(maximum), "set integer %d", INT_MAX);
    assert(!ps_execute(s->core, maximum) && integer == INT_MAX);
    assert(ps_execute(s->core, "set integer 999999999999999999999999") != 0 && integer == INT_MAX);
    assert(!ps_get_parameter(other, "integer"));
    clear();
    type(s, "get integer\nedit\n");
    assert(!strchr(output, '\033') && strstr(output, "TERMINAL_CAPABILITY"));
    assert(ps_history_limit(s->core, 2) == 2);
    ps_history_add(s->core, "one");
    ps_history_add(s->core, "two");
    ps_history_add(s->core, "three");
    assert(ps_history_count(s->core) == 2 && !strcmp(ps_history_get(s->core, 0), "two"));
    ps_history_limit(s->core, 0);
    ps_history_add(s->core, "four");
    assert(!ps_history_count(s->core));
    /* Provider revisions reject a stale edit after an external value changes. */
    assert(s->panel.read(s, 1, &observed));
    requested = observed;
    strcpy(requested.text, "12");
    integer = 99;
    assert(!s->panel.write(s, 1, &requested, observed.revision) && integer == 99);
    assert(s->panel.read(s, 1, &observed));
    assert(s->panel.write(s, 1, &requested, observed.revision) && integer == 12);
    assert(!ps_unregister_parameter(s, "integer") && !ps_get_parameter(s, "integer"));
    ps_destroy(s);
    ps_destroy(other);
    /* Fragmented escapes must never invoke the blocking legacy read callback. */
    c.vt100_enabled = true;
    clear();
    s = ps_create(&c);
    assert(s);
    clear();
    ps_print_prompt(s);
    ps_print_prompt(s);
#ifndef PS_ASCII_ONLY
    assert(!strcmp(output, "\033[32mlegacy> \033[0m\033[32mlegacy> \033[0m"));
#endif
    clear();
    ps_run(s);
    clear();
    ps_run(s);
    assert(strstr(output, "legacy> ") && strstr(output, "\033[?2004h"));
    reads = 0;
    ps_process_char(s, '\033');
    ps_process_char(s, '[');
    ps_process_char(s, 'A');
    assert(reads == 0);
    assert(!ps_register_parameter(s, "label", PS_PARAM_STRING, label, sizeof(label)));
    assert(ps_execute(s->core, "edit") == 0 && s->tui_mode);
    type(s, "\r\025changed");
    /* Registry changes must not retarget or destroy the binding under a draft. */
    assert(ps_unregister_parameter(s, "label") < 0);
    assert(ps_register_parameter(s, "integer", PS_PARAM_INT, &integer, 0) < 0);
    assert(ps_get_parameter(s, "label") && !ps_get_parameter(s, "integer"));
    type(s, "\r");
    assert(!strcmp(label, "changed"));
    assert(strstr(output, "\033[?1049h"));
    ps_destroy(s);
    puts("Legacy adapter: commands, typed bindings, validation, isolation, history, revisions, TUI "
         "and fragmented input passed");
    return 0;
}
