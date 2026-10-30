/* Freestanding tests: the runner provides only a byte sink. */
#include "portable_shell.h"
#include "portable_shell_device.h"
int portable_runtime_checks(void);
int portable_memory_checks(void);
int portable_terminal_checks(void);
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
static int raw_check(ps_session *s, const char *tail, void *ctx) {
    (void)s;
    (void)tail;
    (void)ctx;
    return 9;
}
static unsigned preview_complete(ps_session *s, int argc, const char *const *argv,
                                 const char *prefix, const char **items, unsigned capacity,
                                 void *ctx) {
    unsigned *calls = ctx;
    (void)s; (void)argc; (void)argv; (void)prefix;
    ++*calls;
    if (!capacity) return 0;
    items[0] = "green light";
    return 1;
}
static int device_probe(ps_session *s, const char *name, const char *tail, void *context) {
    unsigned *calls = context;
    ++*calls;
    ps_write(s, name);
    ps_write(s, ":");
    ps_write(s, tail);
    ps_newline(s);
    return 19;
}
typedef struct {
    ps_control_value values[6];
    unsigned writes, mode;
} ControlProbe;
static int probe_read(void *ctx, unsigned id, ps_control_value *v) {
    ControlProbe *p = ctx;
    if (id >= 6)
        return 0;
    *v = p->values[id];
    return 1;
}
static int probe_write(void *ctx, unsigned id, const ps_control_value *v, ps_u32 revision) {
    ControlProbe *p = ctx;
    if (id >= 6 || p->values[id].revision != revision)
        return 0;
    ++p->writes;
    if (p->mode == 2) {
        p->values[id].flags |= PS_CONTROL_PENDING;
        return 2;
    }
    if (p->mode == 3)
        return 0;
    p->values[id] = *v;
    p->values[id].flags = PS_CONTROL_VALID;
    ++p->values[id].revision;
    return 1;
}
int portable_shell_checks(ps_write_fn report, void *ctx) {
    ps_args a;
    ps_session *s;
    ps_config cfg = {sink, &t, 100, 28, 0};
    unsigned i;
    char big[900];
    static const ps_command cmd = {"custom", "", "Test", custom, 0, 0};
    t.failed = t.checks = 0;
    CHECK(portable_runtime_checks(), "freestanding arithmetic helpers match native arithmetic");
    CHECK(portable_memory_checks(),
          "memory ABI and wrappers preserve overlap, bounds and byte semantics");
#ifdef PS_USE_LIBC
    CHECK(eq(ps_memory_backend(), "libc"), "hosted backend identity");
#else
    CHECK(eq(ps_memory_backend(), "portable"), "portable backend identity");
#endif
    {
        const ps_column cols[] = {
            {"ID", 4, 4, 0}, {"Name", 12, 0, 2}, {"State", 8, 12, 1}, {"Value", 10, 0, 1}};
        unsigned widths[4], total, w;
        CHECK(ps_layout_columns(40, cols, 4, widths) == 0,
              "column minimum rejects undersized frame");
        for (w = 47; w <= 238; ++w) {
            total = ps_layout_columns(w, cols, 4, widths);
            if (total != w || widths[0] != 4 || widths[1] < 12 || widths[2] < 8 || widths[2] > 12 ||
                widths[3] < 10 || widths[0] + widths[1] + widths[2] + widths[3] + 13 != w)
                break;
        }
        CHECK(w == 239, "four columns fit every supported width with shared padding");
        CHECK(ps_layout_columns(96, cols, 9, widths) == 0, "column count bounded");
    }
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
    {
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_execute(s, "task add original");
        ps_execute(s, "set sample original");
        ps_execute(s, "board");
        clear();
        feed(s, "\r\025task-draft");
#if PS_WORKSPACE_INLINE_EDIT
        CHECK(has(t.output, "\033[7;22H") && !has(t.output, "Edit task  "),
              "task draft and cursor are in the table cell");
#endif
        ps_key(s, 27);
        clear(); ps_execute(s, "task list");
        CHECK(has(t.output, "original") && !has(t.output, "task-draft"), "inline task cancel");
        feed(s, "\r\025task-saved\r\t\r\025value-draft");
        clear(); ps_execute(s, "get sample");
        CHECK(has(t.output, "original") && !has(t.output, "value-draft"),
              "inline value remains a draft before Enter");
        clear(); ps_resize(s, 42, 14);
        ps_tick(s, 200);
#if PS_WORKSPACE_INLINE_EDIT
        CHECK(has(t.output, "value-draft") && has(t.output, "\033[7;"),
              "inline value remains in its cell after narrow resize");
#endif
        ps_key(s, PS_KEY_LEFT); ps_key(s, PS_KEY_BACKTAB); ps_key(s, PS_KEY_RIGHT);
        ps_key(s, 13);
        clear(); ps_execute(s, "get sample");
        CHECK(has(t.output, "value-draft"), "inline value Enter saves after cursor navigation");
        feed(s, "\r\025discard"); ps_key(s, 27);
        clear(); ps_execute(s, "get sample");
        CHECK(has(t.output, "value-draft") && !has(t.output, "discard"), "inline value Escape cancels");
        clear(); ps_execute(s, "task list");
        CHECK(has(t.output, "task-saved"), "inline task Enter saves");
    }
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
    {
        ps_device_profile profile;
        unsigned calls = 0;
        const char *argv[] = {"echo", "original whitespace is unknown"};
        cfg.flags =
            PS_INTERACTIVE | PS_COLOR | PS_CASE_INSENSITIVE | PS_NO_BUILTINS | PS_LEGACY_KEYS;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        CHECK(ps_device_attach(s, &profile, device_probe, &calls) == 0, "device command profile attach");
        clear();
        ps_start(s);
        CHECK(has(t.output, "> ") && !has(t.output, "PortableShell"),
              "donor prompt replaces brand");
        clear();
        CHECK(ps_execute(s, "SeT Station_SSID  A 'raw value") == 19 && calls == 1 &&
                  has(t.output, "set:Station_SSID  A 'raw value"),
              "raw callback preserves unclosed quotes and spaces");
        clear();
        ps_execute(s, "EcHo  one  'two'");
        CHECK(has(t.output, " one  'two'"), "legacy echo raw semantics");
        CHECK(ps_execute_argv(s, 2, argv) == 2, "raw argv rejects lossy reconstruction");
        CHECK(ps_execute(s, "board") == 2, "replacement registry excludes workbench commands");
        clear();
        ps_execute(s, "HEX -1");
        CHECK(has(t.output, "FFFFFFFF"), "legacy negative hexadecimal");
        CHECK(ps_execute(s, "hex 2147483648") == 2, "profile numeric overflow rejected");
        clear();
        feed(s, "ec\t");
        feed(s, "pending");
        feed(s, "\033[D");
        ps_message(s, PS_STYLE_SUCCESS, "device event");
        feed(s, "!\r");
        CHECK(has(t.output, "device event") &&
                  eq(ps_history_get(s, ps_history_count(s) - 1), "echo pendin!g"),
              "event preserves pending input and cursor after completion");
        clear();
        feed(s, "GET Sta\t");
        CHECK(has(t.output, "Station_"), "case insensitive parameter completion");
        feed(s, "\003");
        clear();
        ps_execute(s, "upper");
        feed(s, "echo MiXeD");
        CHECK(has(t.output, "ECHO MIXED"), "legacy uppercase affects display");
        feed(s, "\033D!\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s) - 1), "echo MiXe!D"),
              "legacy bare ESC cursor sequence retains original argument case");
        ps_execute(s, "noupper");
        {
            static union {
                void *align;
                unsigned char bytes[40000];
            } other_memory;
            ps_device_profile other_profile;
            ps_session *other = ps_init(other_memory.bytes, sizeof other_memory.bytes, &cfg);
            unsigned other_calls = 0;
            ps_device_attach(other, &other_profile, device_probe, &other_calls);
            ps_execute(other, "save");
            ps_execute(other, "exit");
            CHECK(other_calls == 1 && calls == 1 && !ps_exited(s) && ps_exited(other),
                  "independent backend contexts and session lifetime");
        }
        CHECK(ps_set_prompt(s, "bad\033[2J", PS_STYLE_PROMPT) == -1,
              "prompt rejects control injection");
        clear();
        ps_device_greet(s, "Device", "fixture-host", "127.0.0.1");
        CHECK(has(t.output, "Welcome 127.0.0.1 to Device (fixture-host)!"),
              "dynamic transport greeting");
        CHECK(ps_execute(s, "EXIT") == 0 && ps_exited(s), "profile exit reaches host lifecycle");
        cfg.flags = PS_NO_BUILTINS | PS_CASE_INSENSITIVE;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_device_attach(s, &profile, 0, 0);
        clear();
        CHECK(ps_execute(s, "save") == 2 && has(t.output, "Device backend not attached.") &&
                  !has(t.output, "\033"),
              "unbound hardware fails explicitly and batch has no escapes");
    }
    {
        cfg.flags = PS_INTERACTIVE | PS_COLOR | PS_UNICODE;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_execute(s, "task add First");
        ps_execute(s, "task add Second");
        ps_execute(s, "task add Third");
        ps_execute(s, "task add Fourth");
        ps_resize(s, 42, 14);
        ps_execute(s, "board");
        ps_tick(s, 100);
        clear();
        ps_key(s, PS_KEY_PAGEDOWN);
        ps_tick(s, 200);
        CHECK(has(t.output, "Row 2/4"), "page navigation follows narrow viewport capacity");
        clear();
        ps_key(s, PS_KEY_END);
        ps_tick(s, 300);
        CHECK(has(t.output, "Fourth") && has(t.output, "Row 4/4"),
              "last row stays visible with footer");
        clear();
        ps_key(s, PS_KEY_PAGEDOWN);
        ps_tick(s, 400);
        CHECK(has(t.output, "Row 4/4"), "page navigation bounded at last row");
        ps_progress(s, "Caller job", 25, 100);
        ps_key(s, '3');
        clear();
        ps_tick(s, 500);
        CHECK(has(t.output, "Running") && !has(t.output, "Done"),
              "caller progress remains active until total reached");
    }
    {
        static const char *const options[] = {"Manual", "Auto", "Hold"};
        static const ps_control rows[] = {
            {0, "RELAYS", "Relay", PS_CONTROL_TOGGLE, 0, 1, 1, 0, 0},
            {1, "PWM", "Level", PS_CONTROL_LEVEL, 0, 4095, 41, 0, 0},
            {2, "SETUP", "Name", PS_CONTROL_TEXT, 0, 0, 0, 0, 0},
            {3, "SETUP", "Mode", PS_CONTROL_CHOICE, 0, 2, 1, options, 3},
            {4, "ACTIONS", "Stop", PS_CONTROL_ACTION, 0, 0, 0, 0, 0}};
        static const ps_control info[] = {{5, "INFO", "Backend", PS_CONTROL_LABEL, 0, 0, 0, 0, 0}};
        static const ps_control_page pages[] = {{"Control", rows, 5}, {"Info", info, 1}};
        ControlProbe probe = {0};
        ps_panel panel = {"Probe", "Test provider", pages, 2, probe_read, probe_write, &probe};
        unsigned before;
        for (i = 0; i < 6; ++i)
            probe.values[i].flags = PS_CONTROL_VALID;
        cfg.flags = PS_INTERACTIVE | PS_COLOR | PS_UNICODE;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        CHECK(ps_panel_attach(s, &panel) && ps_panel_open(s, 0), "provider pages attach and open");
#if PS_CONTROL_SECTION_ROWS
        ps_key(s, PS_KEY_UP);
        clear();ps_feed(s,"",0);
        CHECK(!has(t.output,"▸ "),"selected category does not mark its unselected first child");
        feed(s, "\r\n");
        clear(); ps_feed(s, "", 0);
        CHECK(!has(t.output, "Relay ") && probe.writes == 0,
              "section CRLF collapses without provider writes");
        ps_key(s, PS_KEY_DOWN);
        ps_key(s, PS_KEY_DOWN);
        ps_key(s, PS_KEY_RIGHT);
        CHECK(probe.values[1].number == 41, "collapsed section skips to next group's value");
        ps_key(s, PS_KEY_UP); ps_key(s, PS_KEY_UP);
        ps_key(s, PS_KEY_RIGHT); ps_key(s, PS_KEY_RIGHT);
        ps_key(s, PS_KEY_DOWN);
        CHECK(probe.writes == 1, "heading expand is idempotent and does not write");
        probe.values[1].number = 0; probe.writes = 0;
#endif
        clear();
        ps_tick(s, 100);
        {
            unsigned groups = 0, at_ = 0;
            for (; at_ + 6 <= t.n; ++at_) {
                if (t.output[at_] == 'R' && t.output[at_+1] == 'E' &&
                    t.output[at_+2] == 'L' && t.output[at_+3] == 'A' &&
                    t.output[at_+4] == 'Y' && t.output[at_+5] == 'S') ++groups;
            }
            CHECK(groups == 1, "wide table does not repeat selected category above table");
        }
        feed(s, " ");
        CHECK(probe.values[0].number == 1 && probe.writes == 1, "toggle dispatches provider write");
#if PS_CONTROL_SECTION_ROWS
        ps_key(s, PS_KEY_DOWN);
#endif
        feed(s, "\033[B\033[C");
        CHECK(probe.values[1].number == 41, "byte arrows adjust PWM without changing tab");
        ps_key(s, PS_KEY_END);
        ps_key(s, PS_KEY_RIGHT);
        CHECK(probe.values[1].number == 4095, "level clamps at maximum");
        ps_key(s, PS_KEY_HOME);
        ps_key(s, PS_KEY_LEFT);
        CHECK(probe.values[1].number == 0, "level clamps at minimum");
        ps_set_table_style(s, PS_TABLE_DEFAULT | PS_TABLE_TABS_INSIDE);
        clear(); ps_feed(s, "", 0);
        CHECK(has(t.output,"\033[4;1H") && has(t.output,"\033[3;2H"),
              "optional tabs occupy the first enclosed table row");
        feed(s, "\r\0252048\r");
        CHECK(probe.values[1].number == 2048, "inline exact value commits");
        before = probe.writes;
        feed(s, "\r\0254096\r");
        CHECK(probe.values[1].number == 2048 && probe.writes == before,
              "invalid draft never reaches backend");
        ps_key(s, 27);
        ps_key(s, PS_KEY_F2);
        feed(s, "\0251000");
        ++probe.values[1].revision;
        ps_panel_refresh(s);
        ps_key(s, 13);
        CHECK(probe.writes == before && probe.values[1].number == 2048,
              "stale dialog revision blocks mutation");
        ps_key(s, 27);
        ps_key(s, PS_KEY_F2);
        feed(s, "\0251000\t\r");
        CHECK(probe.values[1].number == 1000, "dialog Apply commits isolated draft");
        ps_key(s, PS_KEY_F2);
        feed(s, "\0252000\t\t\r");
        CHECK(probe.values[1].number == 1000, "dialog Cancel leaves observed state unchanged");
        probe.mode = 2;
        before = probe.writes;
        ps_key(s, PS_KEY_RIGHT);
        ps_key(s, PS_KEY_RIGHT);
        CHECK(probe.writes == before + 1 && probe.values[1].number == 1000,
              "pending write never becomes applied and suppresses repeat dispatch");
        clear();
        ps_tick(s, 700);
        CHECK(has(t.output, "pending"), "pending observation is rendered");
        probe.mode = 0;
        probe.values[1].flags = PS_CONTROL_VALID | PS_CONTROL_LOCKED;
        before = probe.writes;
        ps_key(s, PS_KEY_LEFT);
        CHECK(probe.writes == before, "locked control cannot dispatch");
        probe.values[1].flags = 0;
        ps_key(s, PS_KEY_END);
        clear();
        ps_tick(s, 800);
        CHECK(probe.writes == before && has(t.output, "Unknown"),
              "unknown is not zero or writable");
        ps_key(s, PS_KEY_DOWN);
#if PS_CONTROL_SECTION_ROWS
        ps_key(s, PS_KEY_DOWN);
#endif
        feed(s, "\rbench\r");
        CHECK(eq(probe.values[2].text, "bench"), "inline text editing");
        ps_key(s, PS_KEY_DOWN);
        ps_key(s, PS_KEY_LEFT);
        CHECK(probe.values[3].number == 2, "choice control cycles backwards");
        ps_key(s, PS_KEY_DOWN);
#if PS_CONTROL_SECTION_ROWS
        ps_key(s, PS_KEY_DOWN);
#endif
        before = probe.writes;
        feed(s, "\r\r");
        CHECK(probe.writes == before, "confirmation defaults to Cancel");
        feed(s, "\r\t\r");
        CHECK(probe.writes == before + 1, "confirmed action callback exactly once");
        clear();
        feed(s, "\t");
        ps_tick(s, 900);
        CHECK(has(t.output, "Backend") && has(t.output, "Row 1/1"), "tab selects provider page");
        before = probe.writes;
        ps_key(s, 13);
        CHECK(probe.writes == before, "label cannot be edited");
#if PS_MOUSE
        ps_key(s,27);
        ps_set_table_style(s,PS_TABLE_DEFAULT|PS_TABLE_CATEGORY_COLUMN);
        ps_resize(s,100,30);
        probe.mode=0;probe.values[0].flags=probe.values[1].flags=PS_CONTROL_VALID;
        probe.values[0].number=0;probe.values[1].number=0;
        ps_panel_open(s,0);ps_feed(s,"",0);
        before=probe.writes;
        feed(s,"\033[<0;75;7M\033[<0;75;7m");
        CHECK(probe.values[0].number==1&&probe.writes==before+1,"mouse value click toggles once, release does not toggle");
        feed(s,"\033[<0;75;8M\033[<32;90;8M\033[<0;90;8m");
        CHECK(probe.values[1].number>0,"mouse press and drag adjusts level through provider");
        before=probe.writes;
        feed(s,"\033[<32;95;8M\033[<0;99999;8M\033[<0;0;8M");
        CHECK(probe.writes==before,"released drag and invalid coordinates cannot write");
        probe.values[1].flags=PS_CONTROL_VALID|PS_CONTROL_LOCKED;
        feed(s,"\033[<0;90;8M\033[<0;90;8m");
        CHECK(probe.writes==before,"mouse cannot bypass locked control");
        clear();feed(s,"\033[<2;75;9M\033[<2;75;9m");
        CHECK(has(t.output,"Apply")&&has(t.output,"Cancel"),"right click opens value dialog");
        feed(s,"\033[<2;75;9M\033[<2;75;9m");
        clear();feed(s,"\033[<0;26;3M\033[<0;26;3m");
        CHECK(has(t.output,"Backend"),"mouse switches tabs");
        ps_key(s,27);clear();feed(s,"echo mouse-clean\r");
        CHECK(has(t.output,"mouse-clean"),"mouse teardown leaves prompt usable");
#endif
        CHECK(ps_panel_attach(s, 0), "provider detaches cleanly");
        cfg.flags = 0;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_panel_attach(s, &panel);
        CHECK(!ps_panel_open(s, 0), "batch refuses alternate screen");
        clear();
        CHECK(ps_execute(s, "controls set 2 'batch label'") == 0 &&
                  ps_execute(s, "controls get 2") == 0 && has(t.output, "batch label") &&
                  !has(t.output, "\033"),
              "batch controls share backend without terminal escapes");
        CHECK(ps_execute(s, "controls set 4 1") == 2, "batch set cannot bypass action dialog");
        CHECK(ps_execute(s, "controls set 3 99") == 2, "choice bounds validated in batch");
        probe.mode = 3;
        CHECK(ps_execute(s, "controls set 0 0") == 2 && probe.values[0].number == 1,
              "provider rejection preserves state");
        panel.count = 9;
        CHECK(!ps_panel_attach(s, &panel), "invalid provider page count rejected");
    }
    {
        ps_command command = {"temporary", "", "Tests", custom, 0, 0};
        /* Unregistering and reusing a slot must not inherit an old raw callback. */
        cfg.flags = 0;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        CHECK(ps_history_limit(s, 2) == 2, "runtime history bound");
        ps_history_add(s, "one");
        ps_history_add(s, "two");
        ps_history_add(s, "three");
        CHECK(ps_history_count(s) == 2 && eq(ps_history_get(s, 0), "two"),
              "history keeps newest entries");
        ps_history_limit(s, 0);
        ps_history_add(s, "disabled");
        CHECK(!ps_history_count(s), "zero history bound disables storage");
        CHECK(ps_register(s, &command) == 0 && ps_unregister(s, "temporary") == 0,
              "command unregister removes registry entry");
        CHECK(ps_execute(s, "temporary") != 0, "removed command cannot dispatch");
        ps_register_raw(s, &command, raw_check);
        ps_unregister(s, "temporary");
        ps_register(s, &command);
        CHECK(ps_execute(s, "temporary") == 7, "reused command slot clears raw callback");
    }
    {
        unsigned calls = 0, before;
        ps_command command = {"preview", "VALUE", "Custom", custom, preview_complete, &calls};
        ps_command unnamed = {"long-custom-command", "ARG", 0, custom, 0, 0};
        cfg.flags = PS_INTERACTIVE | PS_COLOR | PS_UNICODE;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_register(s, &command);
        ps_register(s, &unnamed);
        clear();
        feed(s, "pre");
        CHECK(has(t.output, "view"), "fresh registry ghost without history");
        feed(s, "\033[C\r");
        CHECK(eq(ps_history_get(s, 0), "preview"), "Right accepts registry ghost");
        clear();
        feed(s, "theme lum");
        CHECK(has(t.output, "en"), "argument ghost uses Tab provider");
        feed(s, "\033[F\r");
        CHECK(eq(ps_history_get(s, 1), "theme lumen"), "End accepts argument ghost");
        feed(s, "echo newest retained\r");
        feed(s, "echo n\033[F\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s)-1), "echo newest retained"),
              "End accepts history before registry candidates");
        feed(s, "preview \"gre");
        before = calls;
        feed(s, "\033[C\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s)-1), "preview \"green light\""),
              "quoted ghost appends valid escaped argument");
        CHECK(calls == before + 1, "acceptance uses displayed candidate then redraws once");
        feed(s, "the\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s)-1), "the"),
              "Enter discards ghost instead of implicitly accepting it");
        ps_history_add(s, "echo caf\303\251");
        feed(s, "echo ca\033[F\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s)-1), "echo caf\303\251"),
              "UTF-8 history suffix survives End acceptance");
        feed(s, "th\033[D\033[F\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s)-1), "th"),
              "End inside line only moves to existing end");
        feed(s, "ta\tadd 'still Tab'\r");
        CHECK(eq(ps_history_get(s, ps_history_count(s)-1), "task add 'still Tab'"),
              "ghost queries preserve Tab completion state");
        clear(); ps_key(s, 8);
        CHECK(has(t.output, "\a"), "Backspace at start emits BEL");
        clear(); feed(s, "zzzz\t");
        CHECK(has(t.output, "\a"), "unmatched Tab emits BEL");
        feed(s, "\003");
        for (i = 0; i < PS_LINE - 1; ++i) ps_key(s, 'q');
        clear(); ps_key(s, 'q');
        CHECK(has(t.output, "\a"), "full input buffer emits BEL without accepting overflow");
        feed(s, "\003"); clear(); ps_execute(s, "bell");
        CHECK(eq(t.output, "\a"), "bell command emits exactly byte 7");
        clear(); ps_execute(s, "help");
        CHECK(has(t.output, "Workspace:") && has(t.output, "Shell:") &&
                  has(t.output, "Commands:") && has(t.output, "long-custom-command"),
              "help groups registry headings and preserves full names with null category");
        clear(); ps_execute(s, "help --compact");
        CHECK(!has(t.output, "Workspace:") && has(t.output, "Workspace") &&
                  has(t.output, "long-custom-command"), "compact help remains selectable");
        clear(); ps_execute(s, "help --headings preview");
        CHECK(has(t.output, "Usage: ") && has(t.output, "preview VALUE"),
              "command help shows Usage");
        CHECK(ps_execute(s, "help unknown") == 2, "unknown help target reports failure");
        cfg.flags = PS_INTERACTIVE | PS_NO_BELL | PS_NO_SUGGESTIONS;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        clear(); ps_key(s, 8); ps_execute(s, "bell"); feed(s, "th\033[C\r");
        CHECK(!has(t.output, "\a") && eq(ps_history_get(s,0), "th"),
              "runtime opt-outs mute BEL and disable ghost acceptance");
        feed(s, "ta\tadd optout\r");
        CHECK(eq(ps_history_get(s,1), "task add optout"), "opt-out retains explicit Tab");
        cfg.flags = PS_INTERACTIVE | PS_TELNET;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        clear(); ps_ring_bell(s);
        CHECK(eq(t.output, "\a"), "Telnet passes BEL without negotiation or escaping");
        cfg.flags = 0;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        clear(); ps_execute(s, "bell"); feed(s, "echo batch\n");
        CHECK(eq(t.output, "batch\n"), "batch emits neither BEL nor ghost escapes");
    }
    CHECK(!portable_terminal_checks(), "shared firmware C components");
    {
        cfg.flags = 0;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_history_add(s, "set PASSWORD private-marker");
        ps_history_add(s, "fetch https://user:pass@host/");
        CHECK(!ps_history_count(s), "credential lines are not retained");
        ps_history_add(s, "echo retained-marker");
        ps_history_clear(s);
        CHECK(!ps_history_count(s), "explicit history clear");
        clear();
        ps_write(s, "A\033]52;c;private\aB\033Pprivate\033\\C");
        CHECK(eq(t.output,"ABC"), "private escape payloads are not printed");
        ps_destroy_session(s);
        { unsigned i, nonzero=0;
          for(i=0;i<ps_sizeof_session();++i) nonzero |= memory.bytes[i];
          CHECK(!nonzero,"session destruction scrubs caller storage"); }
    }
    report(ctx, (const ps_byte *)(t.failed ? "Checks failed\n" : "All checks passed\n"),
           t.failed ? 14 : 18);
    return (int)t.failed;
}
