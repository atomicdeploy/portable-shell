#include "portable_shell.h"
/* Link this file into your product, call portable_shell_register_example(session).
   Callback and command descriptors have static lifetime. */
static int greet(ps_session *s, int argc, const char *const *argv, void *context) {
    (void)context;
    ps_write(s, "Hello, ");
    ps_write(s, argc > 1 ? argv[1] : "world");
    ps_newline(s);
    return 0;
}
static unsigned names(ps_session *s, int argc, const char *const *argv, const char *prefix,
                      const char **items, unsigned capacity, void *context) {
    static const char *const choices[] = {"David", "terminal", "firmware"};
    unsigned i, n = 0, j;
    (void)s;
    (void)argc;
    (void)argv;
    (void)context;
    for (i = 0; i < 3 && n < capacity; ++i) {
        for (j = 0; prefix[j] && prefix[j] == choices[i][j]; ++j) {
        }
        if (!prefix[j])
            items[n++] = choices[i];
    }
    return n;
}
void portable_shell_register_example(ps_session *s) {
    static const ps_command c = {"greet", "[name]", "Custom", greet, names, 0};
    ps_register(s, &c);
}
