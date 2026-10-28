/* Freestanding tests: the runner provides only a byte sink. */
#include "portable_shell.h"
typedef struct {
    char output[65536];
    unsigned n, failed, checks;
} Test;
static union {
    void *align;
    unsigned char bytes[40000];
} memory;
static Test t;
static unsigned len(const char *s) {
    unsigned n = 0;
    while (s[n])
        ++n;
    return n;
}
static int eq(const char *a, const char *b) {
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}
static int has(const char *a, const char *b) {
    unsigned i, j;
    for (i = 0; a[i]; ++i) {
        for (j = 0; b[j] && a[i + j] == b[j]; ++j) {
        }
        if (!b[j])
            return 1;
    }
    return !*b;
}
static void sink(void *ctx, const ps_byte *b, unsigned n) {
    Test *x = ctx;
    while (n-- && x->n + 1 < sizeof x->output)
        x->output[x->n++] = (char)*b++;
    x->output[x->n] = 0;
}
static void clear(void) {
    t.n = 0;
    t.output[0] = 0;
}
static void feed(ps_session *s, const char *p) {
    ps_feed(s, p, len(p));
}
static void check(int ok, const char *name, ps_write_fn report, void *ctx) {
    ++t.checks;
    if (!ok) {
        ++t.failed;
        report(ctx, (const ps_byte *)"FAIL ", 5);
        report(ctx, (const ps_byte *)name, len(name));
        report(ctx, (const ps_byte *)"\n", 1);
    }
}
#define CHECK(x, n) check((x), (n), report, ctx)
static int custom(ps_session *s, int argc, const char *const *argv, void *ctx) {
    (void)argc;
    (void)argv;
    (void)ctx;
    ps_write(s, "custom-result");
    return 7;
}
int portable_shell_checks(ps_write_fn report, void *ctx) {
    ps_args a;
    ps_session *s;
    ps_config cfg = {sink, &t, 100, 28, 0};
    unsigned i;
    char big[900];
    static const ps_command cmd = {"custom", "", "Test", custom, 0, 0};
    t.failed = t.checks = 0;
    CHECK(ps_parse("echo 'a b' \"c\\\"d\" '' a\\ b # ignore", &a) == 0, "quoted parse");
    CHECK(a.argc == 5 && eq(a.argv[1], "a b") && eq(a.argv[2], "c\"d") && eq(a.argv[3], "") &&
              eq(a.argv[4], "a b"),
          "parsed values");
    CHECK(ps_parse("echo 'broken", &a) != 0, "unclosed quote");
    CHECK(ps_parse("echo x\\", &a) != 0, "trailing escape");
    s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
    CHECK(s != 0, "init");
    if (!s)
        return 1;
    clear();
    ps_execute(s, "calc 17 * 19");
    CHECK(has(t.output, "323"), "calculator");
    clear();
    CHECK(ps_execute(s, "calc 2147483647 + 1") == 2, "overflow");
    CHECK(ps_execute(s, "calc -2147483648 / -1") == 2, "division overflow");
    CHECK(ps_execute(s, "calc 1 / 0") == 2, "zero divisor");
    clear();
    feed(s, "echo first\r\necho second\n");
    CHECK(eq(t.output, "first\nsecond\n"), "batch CRLF");
    clear();
    feed(s, "echo eof");
    ps_finish(s);
    CHECK(eq(t.output, "eof\n"), "unterminated EOF");
    CHECK(ps_execute(s, "set mode \"quiet run\"") == 0, "set");
    clear();
    ps_execute(s, "get mode");
    CHECK(eq(t.output, "quiet run\n"), "get");
    CHECK(ps_execute(s, "unset mode") == 0, "unset");
    CHECK(ps_execute(s, "get mode") == 2, "missing variable");
    CHECK(ps_execute(s, "task add 'Compile core'") == 0, "task add");
    CHECK(ps_execute(s, "task done 1") == 0, "task done");
    clear();
    ps_execute(s, "task list");
    CHECK(has(t.output, "1 [x] Compile core"), "task state");
    CHECK(ps_execute(s, "task rename 1 'Test core'") == 0, "task rename");
    CHECK(ps_execute(s, "task remove 1") == 0, "task remove");
    clear();
    ps_execute(s, "text slug ' Hello, Byte World! '");
    CHECK(eq(t.output, "hello-byte-world\n"), "slug");
    clear();
    ps_execute(s, "hash hello");
    ps_finish(s);
    CHECK(has(t.output, "4f9f2cab"), "FNV known vector");
    CHECK(ps_register(s, &cmd) == 0 && ps_execute(s, "custom") == 7, "custom callback");
    for (i = 0; i < sizeof big - 1; ++i) {
        big[i] = 'a';
    }
    big[sizeof big - 1] = 0;
    CHECK(ps_parse(big, &a) != 0, "parse limit");
    clear();
    feed(s, big);
    feed(s, "\necho recovered\n");
    CHECK(has(t.output, "line too long") && has(t.output, "recovered"), "batch overflow recovery");
    CHECK(ps_execute(s, "board") == 2, "batch interactive rejection");
    cfg.flags = PS_INTERACTIVE | PS_COLOR | PS_UNICODE;
    s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
    ps_start(s);
    feed(s, "echo hllo\033[D\033[D\033[De\r");
    CHECK(eq(ps_history_get(s, 0), "echo hello"), "arrow insertion");
    feed(s, "\033[A\001\005\r");
    CHECK(ps_history_count(s) == 1, "history dedupe");
    feed(s, "ta\tadd 'Tabbed task'\r");
    CHECK(eq(ps_history_get(s, 1), "task add 'Tabbed task'"), "completion");
    feed(s, "\033[200~echo paste\nnot-executed\033[201~");
    CHECK(ps_history_count(s) == 2, "paste no execute");
    feed(s, "\025");
    feed(s, "\033OQ");
    clear();
    feed(s, "nLive editor\r");
    CHECK(has(t.output, "Live editor"), "board new task");
    feed(s, "\033[Bd");
    feed(s, "\t\t\t");
    CHECK(has(t.output, "Session bytes"), "session tab");
    feed(s, "\033");
    ps_tick(s, 100);
    clear();
    feed(s, "task list\r");
    CHECK(has(t.output, "Live editor"), "board state survives alternate screen");
    feed(s, "\022Tabbed\r\r");
    CHECK(has(ps_history_get(s, ps_history_count(s) - 1), "Tabbed"), "history search");
    clear();
    feed(s, "text \"upp\t'quoted completion'\r");
    CHECK(has(t.output, "QUOTED COMPLETION"), "completion within an open quote");
    CHECK(ps_execute(s, "hash hello --unknown") == 2, "unknown hash option");
    cfg.flags = PS_INTERACTIVE | PS_TELNET;
    s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
    clear();
    ps_start(s);
    CHECK(t.n >= 15 && (unsigned char)t.output[0] == 255, "telnet offer");
    clear();
    {
        const ps_byte bytes[] = {255, 251, 31, 255, 250, 31, 0, 100, 0, 28, 255, 240};
        for (i = 0; i < sizeof bytes; ++i)
            ps_feed(s, bytes + i, 1);
    }
    CHECK(!has(t.output, "unknown command"), "fragmented NAWS");
    clear();
    {
        const ps_byte b[] = {255, 251, 42, 255, 251, 42};
        ps_feed(s, b, sizeof b);
        CHECK(t.n == 3 && (unsigned char)t.output[1] == 254, "unknown option no loop");
    }
    clear();
    feed(s, "echo network\r\n");
    CHECK(ps_history_count(s) == 1, "telnet CRLF one submit");
    report(ctx, (const ps_byte *)(t.failed ? "Checks failed\n" : "All checks passed\n"),
           t.failed ? 14 : 18);
    return (int)t.failed;
}
