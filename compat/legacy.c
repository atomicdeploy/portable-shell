/* Original public API normalized into one canonical shell/panel engine.
   All libc/allocation dependencies are confined to this optional adapter. */
#include "portable_shell_legacy.h"
#include "ps_io.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

static int std_read(void *ctx) {
    (void)ctx;
    return getchar();
}
static int std_char(void *ctx, char c) {
    (void)ctx;
    return putchar((unsigned char)c) < 0 ? -1 : 0;
}
static int std_string(void *ctx, const char *s) {
    (void)ctx;
    return fputs(s, stdout) < 0 ? -1 : 0;
}
void ps_io_stdio_init(PSConfig *c) {
    if (!c)
        return;
    c->read_char = std_read;
    c->write_char = std_char;
    c->write_str = std_string;
    c->io_ctx = NULL;
}
static void emit(void *ctx, const ps_byte *bytes, unsigned n) {
    PortableShell *s = (PortableShell *)ctx;
    unsigned i;
    if (s->io_error)
        return;
    if (s->config.write_char) {
        for (i = 0; i < n; ++i)
            if (s->config.write_char(s->config.io_ctx, (char)bytes[i]) < 0) {
                s->io_error = 1;
                s->running = false;
                return;
            }
    } else if (s->config.write_str) {
        char chunk[128];
        while (n) {
            unsigned k = n < sizeof(chunk) - 1 ? n : (unsigned)sizeof(chunk) - 1;
            memcpy(chunk, bytes, k);
            chunk[k] = 0;
            if (s->config.write_str(s->config.io_ctx, chunk) < 0) {
                s->io_error = 1;
                s->running = false;
                return;
            }
            bytes += k;
            n -= k;
        }
    }
}
static int error(PortableShell *s, const char *code, const char *name, const char *next) {
    ps_print(s, "ERROR blocking ");
    ps_print(s, code);
    ps_print(s, " ");
    ps_print(s, name ? name : "");
    ps_print(s, ": ");
    ps_println(s, next);
    return -1;
}
static int dispatch(ps_session *core, int argc, const char *const *argv, void *ctx) {
    PSCommand *c = (PSCommand *)ctx;
    char storage[PS_LINE];
    char *mutable_args[PS_ARGS + 1];
    unsigned used = 0;
    int i;
    (void)core;
    /* Legacy callbacks own writable argv for this call, never cast away const. */
    for (i = 0; i < argc; ++i) {
        size_t n = strlen(argv[i]) + 1;
        if (n > sizeof(storage) - used)
            return -1;
        mutable_args[i] = storage + used;
        memcpy(mutable_args[i], argv[i], n);
        used += (unsigned)n;
    }
    mutable_args[argc] = NULL;
    return c->func(c->owner, argc, mutable_args);
}
static int valid_name(const char *name) {
    const unsigned char *p = (const unsigned char *)name;
    if (!p || !*p || strlen(name) > 63)
        return 0;
    for (; *p; ++p)
        if (*p <= 32 || *p >= 127 || *p == '"' || *p == '\'' || *p == '\\')
            return 0;
    return 1;
}
int ps_register_command(PortableShell *s, const char *name, const char *help, PSCommandFunc fn) {
    PSCommand *c;
    ps_command command;
    if (!s || !valid_name(name) || !fn)
        return -1;
    for (c = s->commands; c; c = c->next)
        if (!strcmp(c->name, name))
            break;
    if (c) {
        c->func = fn;
        c->help = help ? help : "";
    } else {
        c = (PSCommand *)calloc(1, sizeof(*c));
        if (!c)
            return -1;
        c->name = name;
        c->help = help ? help : "";
        c->func = fn;
        c->owner = s;
        c->next = s->commands;
        s->commands = c;
    }
    command.name = c->name;
    command.usage = c->help;
    command.category = "Commands";
    command.run = dispatch;
    command.complete = NULL;
    command.context = c;
    if (ps_register(s->core, &command)) {
        ps_unregister_command(s, name);
        return -1;
    }
    return 0;
}
int ps_unregister_command(PortableShell *s, const char *name) {
    PSCommand **p;
    if (!s || !name)
        return -1;
    for (p = &s->commands; *p; p = &(*p)->next)
        if (!strcmp((*p)->name, name)) {
            PSCommand *old = *p;
            ps_unregister(s->core, name);
            *p = old->next;
            free(old);
            return 0;
        }
    return -1;
}
PSParameter *ps_get_parameter(PortableShell *s, const char *name) {
    PSParameter *p;
    if (!s || !name)
        return NULL;
    for (p = s->parameters; p; p = p->next)
        if (!strcmp(p->name, name))
            return p;
    return NULL;
}
static int parameter_text(const PSParameter *p, char *out, size_t cap) {
    int n;
    switch (p->type) {
    case PS_PARAM_INT:
        n = snprintf(out, cap, "%d", *(const int *)p->value_ptr);
        break;
    case PS_PARAM_FLOAT:
        n = snprintf(out, cap, "%.9g", (double)*(const float *)p->value_ptr);
        break;
    case PS_PARAM_BOOL:
        n = snprintf(out, cap, "%s", *(const bool *)p->value_ptr ? "true" : "false");
        break;
    case PS_PARAM_STRING: {
        size_t k = 0;
        const char *v = (const char *)p->value_ptr;
        while (k < p->max_size && v[k])
            ++k;
        if (k == p->max_size || k >= cap)
            return 0;
        memcpy(out, v, k + 1);
        return 1;
    }
    default:
        return 0;
    }
    return n >= 0 && (size_t)n < cap;
}
static int parameter_set(PSParameter *p, const char *text) {
    char *end;
    if (!text || !*text) {
        if (p->type != PS_PARAM_STRING)
            return 0;
    }
    errno = 0;
    switch (p->type) {
    case PS_PARAM_INT: {
        long v = strtol(text, &end, 10);
        if (errno || end == text || *end || v < INT_MIN || v > INT_MAX)
            return 0;
        *(int *)p->value_ptr = (int)v;
        return 1;
    }
    case PS_PARAM_FLOAT: {
        float v = strtof(text, &end);
        if (errno || end == text || *end || !isfinite(v))
            return 0;
        *(float *)p->value_ptr = v;
        return 1;
    }
    case PS_PARAM_BOOL: {
        char lowered[8];
        size_t i, n = strlen(text);
        if (n >= sizeof(lowered))
            return 0;
        for (i = 0; i <= n; ++i)
            lowered[i] = text[i] >= 'A' && text[i] <= 'Z' ? (char)(text[i] + 32) : text[i];
        if (!strcmp(lowered, "true") || !strcmp(lowered, "on") || !strcmp(lowered, "1") ||
            !strcmp(lowered, "yes"))
            *(bool *)p->value_ptr = true;
        else if (!strcmp(lowered, "false") || !strcmp(lowered, "off") || !strcmp(lowered, "0") ||
                 !strcmp(lowered, "no"))
            *(bool *)p->value_ptr = false;
        else
            return 0;
        return 1;
    }
    case PS_PARAM_STRING: {
        size_t n = strlen(text);
        if (n >= p->max_size)
            return 0;
        memcpy(p->value_ptr, text, n + 1);
        return 1;
    }
    default:
        return 0;
    }
}
static PSParameter *by_id(PortableShell *s, unsigned id) {
    PSParameter *p;
    for (p = s->parameters; p; p = p->next)
        if (p->slot + 1 == id)
            return p;
    return NULL;
}
static int panel_read(void *ctx, unsigned id, ps_control_value *v) {
    PortableShell *s = (PortableShell *)ctx;
    PSParameter *p = by_id(s, id);
    ps_control_value next = {0}, *last;
    if (!p)
        return 0;
    next.flags = PS_CONTROL_VALID;
    if (!parameter_text(p, next.text, sizeof(next.text))) {
        strcpy(next.text, "Value exceeds inline capacity; use get/set");
        next.flags |= PS_CONTROL_LOCKED;
    }
    if (p->type == PS_PARAM_BOOL)
        next.number = *(bool *)p->value_ptr ? 1u : 0u;
    last = &s->observed[p->slot];
    next.revision = last->revision;
    if (next.flags != last->flags || next.number != last->number || strcmp(next.text, last->text))
        ++next.revision;
    *last = next;
    *v = next;
    return 1;
}
static int panel_write(void *ctx, unsigned id, const ps_control_value *v, ps_u32 revision) {
    PortableShell *s = (PortableShell *)ctx;
    PSParameter *p = by_id(s, id);
    ps_control_value current;
    if (!p || !panel_read(ctx, id, &current) || current.revision != revision ||
        (current.flags & PS_CONTROL_LOCKED))
        return 0;
    return parameter_set(p, p->type == PS_PARAM_BOOL ? (v->number ? "true" : "false") : v->text);
}
static void refresh_panel(PortableShell *s) {
    static const char *const types[] = {"int", "float", "string", "bool"};
    PSParameter *p;
    unsigned n = 0;
    ps_panel_attach(s->core, NULL);
    for (p = s->parameters; p; p = p->next) {
        ps_control *c = &s->controls[n++];
        memset(c, 0, sizeof(*c));
        c->id = p->slot + 1;
        c->group = types[p->type];
        c->label = p->name;
        c->kind = p->type == PS_PARAM_BOOL ? PS_CONTROL_TOGGLE : PS_CONTROL_TEXT;
    }
    s->page.title = "Parameters";
    s->page.controls = s->controls;
    s->page.count = n;
    s->panel.title = "Bound parameters";
    s->panel.status = "Live values";
    s->panel.pages = &s->page;
    s->panel.count = 1;
    s->panel.read = panel_read;
    s->panel.write = panel_write;
    s->panel.context = s;
    if (n)
        ps_panel_attach(s->core, &s->panel);
}
int ps_register_parameter(PortableShell *s, const char *name, PSParamType type, void *ptr,
                          size_t cap) {
    PSParameter *p;
    unsigned slot;
    if (!s || !valid_name(name) || !ptr || type > PS_PARAM_BOOL || type < PS_PARAM_INT ||
        (type == PS_PARAM_STRING && (!cap || !memchr(ptr, 0, cap))))
        return -1;
    p = ps_get_parameter(s, name);
    if (p)
        return p->type == type && p->value_ptr == ptr && p->max_size == cap
                   ? 0
                   : error(s, "DUPLICATE_PARAMETER", name,
                           "unregister the existing binding before replacing it");
    for (slot = 0; slot < PS_LEGACY_PARAMETERS; ++slot)
        if (!by_id(s, slot + 1))
            break;
    if (slot == PS_LEGACY_PARAMETERS)
        return error(s, "PARAMETER_CAPACITY", name, "unregister an unused binding");
    p = (PSParameter *)calloc(1, sizeof(*p));
    if (!p)
        return error(s, "ALLOCATION_FAILED", name, "provide host memory before retrying");
    if (!ps_panel_attach(s->core, NULL)) {
        free(p);
        return error(s, "ACTIVE_EDIT", name, "finish or cancel the current edit first");
    }
    p->name = name;
    p->type = type;
    p->value_ptr = ptr;
    p->max_size = cap;
    p->slot = slot;
    p->next = s->parameters;
    s->parameters = p;
    memset(&s->observed[slot], 0, sizeof(s->observed[slot]));
    refresh_panel(s);
    return 0;
}
int ps_unregister_parameter(PortableShell *s, const char *name) {
    PSParameter **p;
    if (!s || !name)
        return -1;
    for (p = &s->parameters; *p; p = &(*p)->next)
        if (!strcmp((*p)->name, name)) {
            PSParameter *old = *p;
            if (!ps_panel_attach(s->core, NULL))
                return error(s, "ACTIVE_EDIT", name, "finish or cancel the current edit first");
            *p = old->next;
            refresh_panel(s);
            free(old);
            return 0;
        }
    return -1;
}
int ps_cmd_get(PortableShell *s, int argc, char **argv) {
    PSParameter *p;
    char value[PS_LINE];
    if (argc != 2)
        return error(s, "ARGUMENT_COUNT", "get", "get NAME");
    p = ps_get_parameter(s, argv[1]);
    if (!p)
        return error(s, "UNKNOWN_PARAMETER", argv[1], "register the binding first");
    if (!parameter_text(p, value, sizeof(value)))
        return error(s, "VALUE_CAPACITY", p->name,
                     "supply a terminated value shorter than PS_LINE");
    ps_print(s, p->name);
    ps_print(s, " = ");
    ps_println(s, value);
    return 0;
}
int ps_cmd_set(PortableShell *s, int argc, char **argv) {
    PSParameter *p;
    if (argc != 3)
        return error(s, "ARGUMENT_COUNT", "set", "set NAME VALUE (quote spaces)");
    p = ps_get_parameter(s, argv[1]);
    if (!p)
        return error(s, "UNKNOWN_PARAMETER", argv[1], "register the binding first");
    if (!parameter_set(p, argv[2]))
        return error(s, "INVALID_VALUE", p->name,
                     "use a finite in-range number, boolean, or fitting string");
    ps_panel_refresh(s->core);
    return ps_cmd_get(s, 2, argv);
}
int ps_cmd_help(PortableShell *s, int argc, char **argv) {
    int i;
    const char *name = NULL;
    unsigned layout = PS_HELP_HEADINGS;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--compact")) layout = PS_HELP_COMPACT;
        else if (!strcmp(argv[i], "--headings")) layout = PS_HELP_HEADINGS;
        else if (!name) name = argv[i];
        else return error(s, "INVALID_ARGUMENT", "help", "[--headings|--compact] [command]");
    }
    return ps_print_help(s->core, name, layout) ? 0 :
        error(s, "UNKNOWN_COMMAND", name, "use help to list commands");
}
int ps_cmd_clear(PortableShell *s, int argc, char **argv) {
    (void)argc;
    (void)argv;
    ps_clear(s->core);
    return 0;
}
int ps_cmd_edit(PortableShell *s, int argc, char **argv) {
    (void)argc;
    (void)argv;
    if (!s->config.vt100_enabled) {
        ps_println(s, "INFO TERMINAL_CAPABILITY: use get/set without an interactive terminal");
        return 0;
    }
    if (!s->parameters) {
        ps_println(s, "INFO NO_PARAMETERS: register a binding first");
        return 0;
    }
    s->tui_mode = ps_panel_open(s->core, 0) != 0;
    return s->tui_mode ? 0 : -1;
}
PortableShell *ps_create(PSConfig *config) {
    PortableShell *s = (PortableShell *)calloc(1, sizeof(*s));
    ps_config c = {0};
    if (!s)
        return NULL;
    if (config)
        s->config = *config;
    else {
        s->config.history_size = 10;
        s->config.vt100_enabled = true;
    }
    if (!s->config.read_char)
        s->config.read_char = std_read;
    if (!s->config.write_char && !s->config.write_str) {
        s->config.write_char = std_char;
        s->config.write_str = std_string;
    }
    if (!s->config.prompt)
        s->config.prompt = "$ ";
    s->storage = malloc(ps_sizeof_session());
    if (!s->storage) {
        free(s);
        return NULL;
    }
    c.write = emit;
    c.context = s;
    c.columns = 100;
    c.rows = 30;
    c.flags = s->config.vt100_enabled ? PS_INTERACTIVE | PS_COLOR | PS_UNICODE : 0;
    s->core = ps_init(s->storage, ps_sizeof_session(), &c);
    if (!s->core) {
        free(s->storage);
        free(s);
        return NULL;
    }
    ps_set_prompt(s->core, s->config.prompt, PS_STYLE_GREEN);
    if (ps_history_limit(s->core, s->config.history_size) != s->config.history_size)
        ps_println(s, "WARNING HISTORY_CLAMPED: rebuild with larger PS_HISTORY for more entries");
    if (ps_register_command(s, "help", "", ps_cmd_help) ||
        ps_register_command(s, "clear", "", ps_cmd_clear) ||
        ps_register_command(s, "get", "NAME", ps_cmd_get) ||
        ps_register_command(s, "set", "NAME VALUE", ps_cmd_set) ||
        ps_register_command(s, "edit", "", ps_cmd_edit) ||
        ps_register_command(s, "cls", "", ps_cmd_clear)) {
        ps_destroy(s);
        return NULL;
    }
    return s;
}
void ps_destroy(PortableShell *s) {
    if (!s)
        return;
    ps_close(s->core);
    while (s->commands) {
        PSCommand *p = s->commands;
        s->commands = p->next;
        free(p);
    }
    while (s->parameters) {
        PSParameter *p = s->parameters;
        s->parameters = p->next;
        free(p);
    }
    free(s->storage);
    free(s);
}
void ps_run(PortableShell *s) {
    if (!s)
        return;
    s->running = true;
    ps_start(s->core);
    if (!s->config.vt100_enabled)
        ps_print_prompt(s);
    while (s->running && !s->io_error && !ps_exited(s->core)) {
        int c = s->config.read_char(s->config.io_ctx);
        if (c < 0) {
            ps_finish(s->core);
            break;
        }
        ps_process_char(s, (char)c);
    }
    s->running = false;
    ps_close(s->core);
}
void ps_stop(PortableShell *s) {
    if (s)
        s->running = false;
}
int ps_process_char(PortableShell *s, char c) {
    if (!s || s->io_error)
        return -1;
    ps_feed(s->core, &c, 1);
    return s->io_error ? -1 : ps_status(s->core);
}
void ps_print(PortableShell *s, const char *text) {
    if (s && text)
        ps_write(s->core, text);
}
void ps_println(PortableShell *s, const char *text) {
    if (s) {
        ps_print(s, text);
        ps_newline(s->core);
    }
}
void ps_print_prompt(PortableShell *s) {
    if (s) {
        ps_vt100_color(s, 32, -1);
        ps_print(s, s->config.prompt);
        ps_vt100_reset(s);
    }
}
/* Trusted terminal controls only; untrusted text goes through ps_print. */
static void vt(PortableShell *s, const char *text) {
    if (s && s->config.vt100_enabled)
        emit(s, (const ps_byte *)text, (unsigned)strlen(text));
}
static void vt_number(PortableShell *s, int n, char suffix) {
    char b[32];
    if (n < 0)
        return;
    snprintf(b, sizeof(b), "\033[%d%c", n, suffix);
    vt(s, b);
}
void ps_vt100_clear_screen(PortableShell *s) {
    vt(s, "\033[2J");
}
void ps_vt100_clear_line(PortableShell *s) {
    vt(s, "\033[2K");
}
void ps_vt100_cursor_home(PortableShell *s) {
    vt(s, "\033[H");
}
void ps_vt100_cursor_move(PortableShell *s, int n) {
    if (n >= 0 && n < INT_MAX)
        vt_number(s, n + 1, 'G');
}
void ps_vt100_cursor_left(PortableShell *s, int n) {
    if (n > 0)
        vt_number(s, n, 'D');
}
void ps_vt100_cursor_right(PortableShell *s, int n) {
    if (n > 0)
        vt_number(s, n, 'C');
}
void ps_vt100_color(PortableShell *s, int fg, int bg) {
#ifdef PS_ASCII_ONLY
    (void)s;
    (void)fg;
    (void)bg;
#else
    char b[32];
    if (fg < 0 || fg > 107 || bg > 107)
        return;
    if (bg >= 0)
        snprintf(b, sizeof(b), "\033[%d;%dm", fg, bg);
    else
        snprintf(b, sizeof(b), "\033[%dm", fg);
    vt(s, b);
#endif
}
void ps_vt100_reset(PortableShell *s) {
    vt(s, "\033[0m");
}
void ps_bell(PortableShell *s) {
    if (s)
        emit(s, (const ps_byte *)"\a", 1);
}
