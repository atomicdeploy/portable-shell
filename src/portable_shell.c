#include "portable_undo.h"
#include "portable_style.h"
#include "portable_segments.h"
#include "portable_parse.h"
#include "portable_shell.h"
#ifdef PS_USE_LIBC
#define PS_TERMINAL_USE_LIBC
#endif
#include "portable_terminal.h"

#include "portable_primitives.h"
static unsigned min(unsigned a, unsigned b) {
    return a < b ? a : b;
}
static unsigned dec(char *p, ps_u32 v) {
    char b[11];
    unsigned n = 0, i;
    do {
        b[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    for (i = 0; i < n; ++i)
        p[i] = b[n - i - 1];
    p[n] = 0;
    return n;
}
static int integer(const char *p, int *v) {
    unsigned n = 0, lim = 2147483647u;
    int neg = 0;
    if (*p == '-') {
        neg = 1;
        ++p;
        lim++;
    } else if (*p == '+')
        ++p;
    if (!*p)
        return 0;
    while (*p) {
        unsigned d = (unsigned)(*p++ - '0');
        if (d > 9 || n > (lim - d) / 10)
            return 0;
        n = n * 10 + d;
    }
    *v = neg ? (n == 2147483648u ? (-2147483647 - 1) : -(int)n) : (int)n;
    return 1;
}
static unsigned prev(const char *p, unsigned i) {
    if (i) {
        --i;
        while (i && ((unsigned char)p[i] & 192) == 128)
            --i;
    }
    return i;
}
static unsigned next(const char *p, unsigned i) {
    if (p[i]) {
        ++i;
        while (p[i] && ((unsigned char)p[i] & 192) == 128)
            ++i;
    }
    return i;
}
static unsigned rune(const char *p, unsigned *used) {
    unsigned char c = (unsigned char)*p;
    unsigned n, v, i;
    if (c < 128) {
        *used = 1;
        return c;
    }
    n = c >= 240 ? 4 : c >= 224 ? 3 : c >= 194 ? 2 : 1;
    v = c & ((1u << (7 - n)) - 1);
    if (n == 1) {
        *used = 1;
        return 0xfffd;
    }
    for (i = 1; i < n; ++i) {
        if (!p[i] || ((unsigned char)p[i] & 192) != 128) {
            *used = 1;
            return 0xfffd;
        }
        v = (v << 6) | ((unsigned char)p[i] & 63);
    }
    *used = n;
    if (v > 0x10ffff || (v >= 0xd800 && v <= 0xdfff) || (n == 2 && v < 128) ||
        (n == 3 && v < 2048) || (n == 4 && v < 65536))
        return 0xfffd;
    return v;
}
static unsigned width(unsigned c) {
    if ((c >= 0x300 && c <= 0x36f) || (c >= 0xfe00 && c <= 0xfe0f) || c == 0x200d)
        return 0;
    return (c >= 0x1100 && (c <= 0x115f || (c >= 0x2e80 && c <= 0xa4cf) ||
                            (c >= 0xac00 && c <= 0xd7a3) || (c >= 0xf900 && c <= 0xfaff) ||
                            (c >= 0xff01 && c <= 0xff60) || (c >= 0x1f300 && c <= 0x1faff)))
               ? 2
               : 1;
}
static unsigned cells(const char *p, unsigned bytes, unsigned flags) {
    unsigned i = 0, n, w = 0;
    while (i < bytes && p[i]) {
        unsigned c = rune(p + i, &n);
        if (i + n > bytes)
            break;
        w += (flags & PS_UNICODE) ? width(c) : 1;
        i += n;
    }
    return w;
}

typedef struct {
    char title[PS_VALUE];
    unsigned id, done, selected;
} Task;
typedef struct {
    char key[32], value[PS_VALUE];
} Variable;
typedef struct {
    unsigned key;
    ps_key_fn fn;
    void *context;
    char command[64];
} Binding;
typedef struct {
    char name[40];
    unsigned done, total, active, cancelled;
    ps_u32 hash;
} Job;
struct ps_session {
#if PS_DASHBOARD
    unsigned dashboard;
#endif
#if PS_FORM
    const ps_form *form;
    unsigned form_index, form_choice, form_top;
    const char *form_error;
#endif
    char document[PS_EDITOR ? PS_EDITOR_CAPACITY : 1], document_undo[PS_EDITOR ? PS_EDITOR_CAPACITY : 1], scratch[PS_EDITOR ? PS_EDITOR_CAPACITY : 1];
    const char *document_title;
    ps_editor_save_fn document_save;
    void *document_context;
    unsigned document_open, document_length, document_pos, document_top;
    unsigned document_dirty, document_dialog, document_undo_valid, document_undo_pos;
    unsigned document_undo_dirty;
    unsigned document_style;
    const char *document_notice;
    const ps_menu *menu_stack[PS_MENU_DEPTH];
    unsigned menu_depth, menu_length, menu_bad;
    char menu_input[4];
#if PS_MENU_LIST_VIEW
    char menu_filter[64];
    unsigned menu_filtering, menu_filter_pos, menu_help;
#endif
    unsigned menu_style, menu_selected, menu_top, menu_screen, menu_x, menu_width, menu_slots;
    ps_menu_description_fn menu_description;
    void *menu_description_context;
    const ps_file_node *file_nodes;
    const char *file_title;
    ps_file_pick_fn file_pick;
    void *file_context;
    unsigned file_count, file_selected, file_top;
    unsigned char file_expanded[128];
    unsigned char file_loaded[128];
    ps_file_load_fn file_load;
    unsigned file_layout, file_icons, file_directory, file_pane, file_list_selected, file_list_top;
    const char *file_notice;
    ps_config io;
    unsigned theme; /* Legacy palette IDs 0..3; semantic roles are the default (4). */
    unsigned table_style;
    char selection_marker[32];
    const ps_panel *panel;
    ps_panel local_panel;
    unsigned control_view, dialog, dialog_focus;
    unsigned control_header, control_edit_row;
    ps_byte control_closed[PS_CONTROL_SECTION_ROWS ? 8 : 1][PS_CONTROL_SECTION_ROWS ? 16 : 1];
    ps_u32 draft_revision, local_revision;
    unsigned local_values[7];
    char local_label[64];
#if PS_HELP_DETAILS
    const ps_help_document *help_documents;
    unsigned help_document_count;
#endif
    ps_command commands[PS_COMMANDS];
    ps_raw_command_fn raw_commands[PS_COMMANDS];
    char prompt_text[256];
    unsigned prompt_part_count, prompt_rows_drawn;
    struct { unsigned end, style; } prompt_parts[16];
    unsigned custom_prompt, uppercase_echo, executing;
    unsigned command_count;
    ps_line_fn line_executor;
    void *line_context;
    ps_command_fn executor;
    void *executor_context;
    Binding bindings[24];
    unsigned binding_count;
#if PS_UNDO_DEPTH
    ps_undo undo;
    char undo_frames[PS_UNDO_DEPTH+1][PS_LINE];
    unsigned undo_positions[PS_UNDO_DEPTH+1];
#endif
    char line[PS_LINE], saved[PS_LINE], kill[PS_LINE], suggestion[PS_LINE];
    unsigned len, pos;
    char history[PS_HISTORY][PS_LINE];
    unsigned history_count, history_head, history_nav, history_limit;
    ps_history_filter_fn history_filter;
    void *history_context;
    char notice[128], filter[64], edit[PS_VALUE];
    unsigned editing, editlen, editpos;
    Task tasks[PS_ROWS];
    unsigned task_count, next_id;
    Variable vars[PS_ROWS];
    unsigned var_count;
    Job job;
    unsigned tab, selected, scroll, board, dirty, exit, started;
    unsigned canvas_kind, canvas_key, canvas_columns, canvas_rows;
    int status;
    unsigned esc, esc_len;
    char escbuf[32];
    unsigned mouse_edit_x, mouse_edit_y, mouse_edit_start, mouse_edit_width;
    unsigned mouse_tab_left[8], mouse_tab_right[8];
    unsigned mouse_drag;
    ps_u32 esc_time, now, last_paint;
    unsigned paste, paste_match, suppress_lf, overflow;
    unsigned search, search_index;
    char query[64];
    unsigned querylen;
    const char *matches[PS_COMMANDS];
    unsigned match_count, match_index, completion, start, end;
    ps_telnet telnet;
    unsigned bytes_in, bytes_out, executions;
    ps_u32 hash_work;
    char job_data[PS_LINE];
    unsigned job_length;
};
static void raw(ps_session *s, const char *p, unsigned n) {
    if (!n)
        return;
    s->bytes_out += n;
    if (!(s->io.flags & PS_TELNET)) {
        s->io.write(s->io.context, (const ps_byte *)p, n);
        return;
    }
    while (n--) {
        ps_byte b = (ps_byte)*p++;
        if (b == '\r') {
            ps_byte pair[2] = {13, 0};
            if (n && *p == '\n') {
                pair[1] = 10;
                ++p;
                --n;
            }
            s->io.write(s->io.context, pair, 2);
        } else if (b == '\n') {
            ps_byte pair[2] = {13, 10};
            s->io.write(s->io.context, pair, 2);
        } else {
            s->io.write(s->io.context, &b, 1);
            if (b == 255)
                s->io.write(s->io.context, &b, 1);
        }
    }
}
static void out(ps_session *s, const char *p) {
    raw(s, p, length(p));
}
static void ansi(ps_session *s, const char *p) {
    if (s->io.flags & PS_INTERACTIVE)
        out(s, p);
}
static void color(ps_session *s, const char *p) {
    if ((s->io.flags & (PS_INTERACTIVE | PS_COLOR)) == (PS_INTERACTIVE | PS_COLOR))
        out(s, p);
}
static void frame_begin(ps_session *s) {
    if(PS_SYNC_OUTPUT&&!(s->io.flags&PS_NO_SYNC_OUTPUT))ansi(s,"\033[?2026h");
}
static void frame_end(ps_session *s) {
    if(PS_SYNC_OUTPUT&&!(s->io.flags&PS_NO_SYNC_OUTPUT))ansi(s,"\033[?2026l");
}
static void canvas_begin(ps_session *s,unsigned kind,unsigned key) {
    frame_begin(s);ansi(s,"\033[?25l\033[H");
    if(s->canvas_kind!=kind||s->canvas_key!=key||s->canvas_columns!=s->io.columns||s->canvas_rows!=s->io.rows)
        ansi(s,"\033[2J");
    s->canvas_kind=kind;s->canvas_key=key;s->canvas_columns=s->io.columns;s->canvas_rows=s->io.rows;
}
static void reset(ps_session *s) {
    ansi(s, "\033[0m");
    if (s->board) {
        if(s->theme==PS_THEME_DEFAULT){color(s,ps_style_base(s->theme,(s->io.flags&PS_TRUECOLOR)!=0));return;}
        if (s->io.flags & PS_TRUECOLOR)
            color(s, s->theme == 3   ? "\033[38;2;171;178;191m\033[48;2;40;44;52m"
                     : s->theme == 2 ? "\033[38;2;220;228;238m\033[48;2;12;18;29m"
                     : s->theme == 1 ? "\033[38;2;255;226;232m\033[48;2;30;20;25m"
                                     : "\033[38;2;241;231;255m\033[48;2;23;19;31m");
        else
            color(s, "\033[38;5;255;48;5;234m");
    }
}
static void ink(ps_session *s, unsigned kind) {
    color(s, ps_style_ink(s->theme, (s->io.flags & PS_TRUECOLOR) != 0,
                         (s->table_style & PS_TABLE_RED_BORDER) != 0, kind));
}
static void surface(ps_session *s, unsigned active) {
    color(s, ps_style_surface(s->theme, (s->io.flags & PS_TRUECOLOR) != 0, active));
}
static void spaces(ps_session *s, unsigned n) {
    while (n--)
        out(s, " ");
}
PS_API void ps_number(ps_session *s, ps_u32 n) {
    char b[11];
    dec(b, n);
    out(s, b);
}
static void signed_number(ps_session *s, int n) {
    if (n < 0) {
        out(s, "-");
        ps_number(s, 0u - (unsigned)n);
    } else
        ps_number(s, (unsigned)n);
}
PS_API void ps_write(ps_session *s, const char *text) {
    unsigned i = 0, n, c;
    while (text[i]) {
        if ((unsigned char)text[i] == 27) {
            i += (unsigned)ps_escape_span(text + i, (size_t)-1);
            continue;
        }
        c = rune(text + i, &n);
        if (c >= 32 && c != 127 && !(c >= 128 && c < 160)) {
            if (c == 0xfffd || (!(s->io.flags & PS_UNICODE) && c > 126))
                out(s, "?");
            else
                raw(s, text + i, n);
        }
        i += n;
    }
}
PS_API void ps_newline(ps_session *s) {
    s->prompt_rows_drawn=0;
    out(s, (s->io.flags & PS_INTERACTIVE) ? "\r\n" : "\n");
}
static void at(ps_session *s, unsigned r, unsigned c) {
    out(s, "\033[");
    ps_number(s, r);
    out(s, ";");
    ps_number(s, c);
    out(s, "H");
}
static void field(ps_session *s, const char *p, unsigned cap) {
    unsigned i = 0, n, w = 0, k, c;
    while (p[i]) {
        if ((unsigned char)p[i] == 27) {
            i += (unsigned)ps_escape_span(p + i, (size_t)-1);
            continue;
        }
        c = rune(p + i, &n);
        k = (s->io.flags & PS_UNICODE) ? width(c) : 1;
        if (w + k > cap)
            break;
        if (c >= 32 && c != 127 && !(c >= 128 && c < 160)) {
            if (c == 0xfffd || (!(s->io.flags & PS_UNICODE) && c > 126))
                out(s, "?");
            else
                raw(s, p + i, n);
            w += k;
        }
        i += n;
    }
    spaces(s, cap - w);
}
static void footer_field(ps_session *s,const char *p,unsigned cap,unsigned status){
    char layout[384];unsigned used=0,first=1;
    (void)status;
    ps_style_footer_format(layout,sizeof layout,p,cap,PS_FOOTER_GRID&&!(s->table_style&PS_TABLE_COMPACT_FOOTER));p=layout;
    reset(s);ink(s,0);
    while(*p&&used<cap){
        char token[96];unsigned n=0,w,styled;
        if(*p==' '){out(s," ");++p;++used;continue;}
        while(p[n]&&p[n]!=' '&&n+1<sizeof token){token[n]=p[n];++n;}
        token[n]=0;
        w=min(cells(token,n,s->io.flags),cap-used);
        styled=PS_FOOTER_KEYCAPS&&!(s->table_style&PS_TABLE_PLAIN_KEYS)&&ps_style_key_token(token,n);
        if(styled)ansi(s,!(s->io.flags&PS_COLOR)?"\033[1;7m":first&&(s->table_style&PS_TABLE_PROMINENT_KEYS)?PS_KEYCAP_PROMINENT_STYLE:PS_KEYCAP_STYLE);
        field(s,token,w);
        if(styled){ansi(s,"\033[0m");reset(s);ink(s,0);first=0;}
        used+=w;p+=n;
    }
    spaces(s,cap-used);
}
static void notice(ps_session *s, const char *p) {
    str(s->notice, p, sizeof s->notice);
    s->dirty = 1;
}
static void prompt(ps_session *s);
static void render(ps_session *s);
static int builtin(ps_session *s, int argc, const char *const *argv, void *ctx);
static unsigned fold(unsigned c) {
    return c >= 'A' && c <= 'Z' ? c + 32 : c;
}
static int name_match(ps_session *s, const char *a, const char *b, int partial) {
    while (*b) {
        unsigned x = (unsigned char)*a++, y = (unsigned char)*b++;
        if (s->io.flags & PS_CASE_INSENSITIVE) {
            x = fold(x);
            y = fold(y);
        }
        if (x != y)
            return 0;
    }
    return partial || !*a;
}
static void style(ps_session *s, unsigned value) {
    if(value==PS_STYLE_BLUE){color(s,"\033[94m");return;}
    if(s->theme==PS_THEME_DEFAULT && value){
        static const unsigned roles[]={PS_INK_TEXT,PS_INK_HEADING,PS_INK_SUCCESS,PS_INK_WARNING,PS_INK_ERROR,PS_INK_SUCCESS,PS_INK_MUTED,PS_INK_COMMAND,PS_INK_HEADING};
        ink(s,roles[value<=PS_STYLE_BLUE?value:0]);return;
    }
    static const char *const colors[] = {"\033[0m", "\033[1;93m", "\033[92m", "\033[93m",
                                         "\033[91m", "\033[32m", "\033[90m", "\033[95m", "\033[94m"};
    color(s, colors[value <= PS_STYLE_BLUE ? value : 0]);
}
PS_API int ps_set_prompt_parts(ps_session *s,const ps_prompt_part *parts,unsigned count) {
    unsigned i,j,n=0,lines=1;
    if(count>16 || (count&&!parts))return -1;
    for(i=0;i<count;++i){
        if(!parts[i].text || parts[i].style>PS_STYLE_BLUE)return -1;
        for(j=0;parts[i].text[j];++j){
            unsigned c=(unsigned char)parts[i].text[j];
            if(c=='\n'){if(++lines>4)return -1;}
            else if(c<32||c==127)return -1;
            if(++n>=sizeof s->prompt_text)return -1;
        }
    }
    n=0;
    for(i=0;i<count;++i){
        for(j=0;parts[i].text[j];++j)s->prompt_text[n++]=parts[i].text[j];
        s->prompt_parts[i].end=n;s->prompt_parts[i].style=parts[i].style;
    }
    s->prompt_text[n]=0;s->prompt_part_count=count;s->custom_prompt=count!=0;
    if(s->started&&!s->executing)prompt(s);
    return 0;
}
PS_API int ps_set_prompt(ps_session *s,const char *text,unsigned value) {
    ps_prompt_part part={text,value};
    return ps_set_prompt_parts(s,text?&part:0,text?1:0);
}
PS_API void ps_set_uppercase_echo(ps_session *s, int enabled) {
    s->uppercase_echo = enabled != 0;
}
PS_API void ps_message(ps_session *s, unsigned value, const char *text) {
    if (s->board) {
        notice(s, text);
        return;
    }
    if (s->started && !s->executing)
        ansi(s, "\r\033[2K");
    style(s, value);
    ps_write(s, text);
    reset(s);
    ps_newline(s);
    if (s->started && !s->executing)
        prompt(s);
}
PS_API void ps_clear(ps_session *s) {
    s->prompt_rows_drawn=0;ansi(s, "\033[2J\033[H");
    s->dirty = 1;
}
PS_API void ps_request_exit(ps_session *s) {
    s->exit = 1;
    ps_close(s);
}

PS_API int ps_parse(const char *line, ps_args *a) {
    ps_parse_result result;
    int status;
    zero(a, sizeof *a);
    status = ps_parse_tokens(line, PS_LINE, a->storage, sizeof(a->storage),
                             a->argv, PS_ARGS, &result);
    a->argc = result.argc;
    a->error = result.error;
    a->error_at = (unsigned)result.error_at;
    return status;
}
PS_API const char *ps_option(int argc, const char *const *argv, const char *name) {
    int i;
    unsigned n = length(name);
    for (i = 1; i < argc; ++i) {
        const char *p = argv[i];
        if (equal(p, "--"))
            break;
        if (p[0] == '-' && p[1] == '-' && prefix(p + 2, name)) {
            if (p[n + 2] == '=')
                return p + n + 3;
            if (!p[n + 2])
                return (i + 1 < argc && argv[i + 1][0] != '-') ? argv[i + 1] : "true";
        }
    }
    return 0;
}
PS_API int ps_register(ps_session *s, const ps_command *c) {
    unsigned i;
    if (!c || !c->name || !*c->name || !c->run)
        return -1;
    for (i = 0; i < s->command_count; ++i)
        if (name_match(s, c->name, s->commands[i].name, 0)) {
            s->commands[i] = *c;
            s->raw_commands[i] = 0;
            return 0;
        }
    if (s->command_count == PS_COMMANDS)
        return -1;
    s->raw_commands[s->command_count] = 0;
    s->commands[s->command_count++] = *c;
    return 0;
}
PS_API unsigned ps_command_count(const ps_session *s) {
    return s ? s->command_count : 0;
}
PS_API const ps_command *ps_command_at(const ps_session *s, unsigned index) {
    return s && index < s->command_count ? &s->commands[index] : 0;
}
PS_API int ps_unregister(ps_session *s, const char *name) {
    unsigned i;
    if (!s || !name)
        return -1;
    for (i = 0; i < s->command_count; ++i) {
        if (name_match(s, name, s->commands[i].name, 0)) {
            for (; i + 1 < s->command_count; ++i) {
                s->commands[i] = s->commands[i + 1];
                s->raw_commands[i] = s->raw_commands[i + 1];
            }
            --s->command_count;
            s->completion = s->match_count = s->match_index = 0;
            s->dirty = 1;
            return 0;
        }
    }
    return -1;
}
static int raw_argv_error(ps_session *s, int argc, const char *const *argv, void *context) {
    (void)argc;
    (void)argv;
    (void)context;
    ps_message(s, PS_STYLE_ERROR, "Raw command requires ps_execute with its original text");
    return 2;
}
PS_API int ps_register_raw(ps_session *s, const ps_command *metadata, ps_raw_command_fn run) {
    ps_command c;
    unsigned i;
    if (!metadata || !run)
        return -1;
    c = *metadata;
    c.run = raw_argv_error;
    if (ps_register(s, &c))
        return -1;
    for (i = 0; i < s->command_count; ++i)
        if (name_match(s, c.name, s->commands[i].name, 0)) {
            s->raw_commands[i] = run;
            break;
        }
    return 0;
}
PS_API int ps_bind(ps_session *s, unsigned key, ps_key_fn fn, void *context) {
    unsigned i;
    for (i = 0; i < s->binding_count; ++i)
        if (s->bindings[i].key == key)
            break;
    if (i == 24)
        return -1;
    if (i == s->binding_count)
        ++s->binding_count;
    zero(&s->bindings[i], sizeof(Binding));
    s->bindings[i].key = key;
    s->bindings[i].fn = fn;
    s->bindings[i].context = context;
    return 0;
}
PS_API unsigned ps_history_count(const ps_session *s) {
    return s->history_count;
}
PS_API const char *ps_history_get(const ps_session *s, unsigned index) {
    if (index >= s->history_count)
        return "";
    return s->history[(s->history_head + PS_HISTORY - s->history_count + index) % PS_HISTORY];
}
PS_API int ps_history_add(ps_session *s, const char *line) {
    if (!line || !*line || length(line) >= PS_LINE)
        return -1;
    if (!s->history_limit || !ps_history_safe(line, PS_LINE))
        return 0;
    if (s->history_filter && !s->history_filter(s->history_context, line))
        return 0;
    if (s->history_count && equal(ps_history_get(s, s->history_count - 1), line))
        return 0;
    ps_secure_clear(s->history[s->history_head], PS_LINE);
    str(s->history[s->history_head], line, PS_LINE);
    s->history_head = (s->history_head + 1) % PS_HISTORY;
    if (s->history_count < s->history_limit)
        ++s->history_count;
    return 0;
}
PS_API void ps_history_clear(ps_session *s) {
    ps_secure_clear(s->history, sizeof s->history);
    ps_secure_clear(s->saved, sizeof s->saved);
    ps_secure_clear(s->suggestion, sizeof s->suggestion);
    s->history_count = s->history_head = s->history_nav = 0;
}
PS_API void ps_history_filter(ps_session *s, ps_history_filter_fn filter, void *context) {
    ps_history_clear(s);
    s->history_filter = filter;
    s->history_context = context;
}
/* Explicit destruction scrubs the complete caller-owned session after restoring
   terminal modes. ps_close remains a display lifecycle operation. */
PS_API void ps_destroy_session(ps_session *s) {
    if (!s)
        return;
    ps_close(s);
    ps_secure_clear(s, sizeof *s);
}
PS_API unsigned ps_history_limit(ps_session *s, unsigned limit) {
    if (!s)
        return 0;
    s->history_limit = limit > PS_HISTORY ? PS_HISTORY : limit;
    if (!s->history_limit)
        ps_history_clear(s);
    while (s->history_count > s->history_limit) {
        unsigned oldest = (s->history_head + PS_HISTORY - s->history_count) % PS_HISTORY;
        ps_secure_clear(s->history[oldest], PS_LINE);
        --s->history_count;
    }
    if (s->history_count > s->history_limit)
        s->history_count = s->history_limit;
    s->history_nav = 0;
    return s->history_limit;
}
static const ps_command builtins[] = {
    {"controls", "[open [page]|dashboard|list|get ID|set ID VALUE]", "Workspace", builtin, 0, 0},
    {"help", "[--headings|--compact] [command]", "Shell", builtin, 0, 0},
    {"board", "[tasks|variables|jobs|session]", "Workspace", builtin, 0, 0},
    {"edit", "[--framed]", "Workspace", builtin, 0, 0},
    {"menu", "[--plain|--list|--no-numbers|--details] [NUMBER ...]", "Workspace", builtin, 0, 0},
    {"files", "", "Workspace", builtin, 0, 0},
    {"task", "add TITLE | done ID | remove ID | rename ID TITLE | list", "Workspace", builtin, 0,
     0},
    {"set", "KEY VALUE", "Workspace", builtin, 0, 0},
    {"get", "[KEY]", "Workspace", builtin, 0, 0},
    {"unset", "KEY", "Workspace", builtin, 0, 0},
    {"text", "upper|lower|trim|slug TEXT", "Tools", builtin, 0, 0},
    {"calc", "INTEGER +|-|*|/|% INTEGER", "Tools", builtin, 0, 0},
    {"hash", "TEXT [--rounds N]", "Tools", builtin, 0, 0},
    {"jobs", "[cancel]", "Tools", builtin, 0, 0},
    {"bell", "", "Shell", builtin, 0, 0},
    {"echo", "[TEXT ...]", "Shell", builtin, 0, 0},
    {"args", "[ARGUMENT ...]", "Shell", builtin, 0, 0},
    {"history", "[clear]", "Shell", builtin, 0, 0},
    {"bind", "F1|F2|F3|F4 COMMAND", "Shell", builtin, 0, 0},
    {"theme", "default|cyan|violet|red|onedark", "Appearance", builtin, 0, 0},
    {"display", "color rgb|ansi|mono; glyphs unicode|ascii", "Terminal", builtin, 0, 0},
    {"prompt", "reset | STYLE TEXT [STYLE TEXT ...]", "Terminal", builtin, 0, 0},
    {"segments", "TEXT [--large] [--red] [--mono] [--ascii]", "Tools", builtin, 0, 0},
    {"clear", "", "Terminal", builtin, 0, 0},
    {"status", PS_REPORT?"[--report]":"", "Terminal", builtin, 0, 0},
    {"exit", "", "Shell", builtin, 0, 0}};
PS_API unsigned ps_sizeof_session(void) {
    return sizeof(ps_session);
}
PS_API const char *ps_memory_backend(void) {
#ifdef PS_USE_LIBC
    return "libc";
#else
    return "portable";
#endif
}
PS_API unsigned ps_layout_columns(unsigned width, const ps_column *columns, unsigned count,
                                  unsigned *widths) {
    unsigned i, used, left, changed, step;
    if (!columns || !widths || !count || count > 8 || width > 240)
        return 0;
    used = 3 * count + 1;
    for (i = 0; i < count; ++i) {
        if (!columns[i].minimum || columns[i].minimum > 240 || columns[i].weight > 16 ||
            (columns[i].maximum && columns[i].maximum < columns[i].minimum))
            return 0;
        widths[i] = columns[i].minimum;
        used += widths[i];
    }
    if (used > width)
        return 0;
    left = width - used;
    do {
        changed = 0;
        for (i = 0; i < count && left; ++i)
            for (step = 0; step < columns[i].weight && left; ++step)
                if (!columns[i].maximum || widths[i] < columns[i].maximum) {
                    ++widths[i];
                    --left;
                    ++changed;
                }
    } while (left && changed);
    return width - left;
}
PS_API void ps_set_selection_marker(ps_session *s,const char *marker) {
    str(s->selection_marker,marker?marker:"",sizeof s->selection_marker);
    s->dirty=1;
}
PS_API void ps_set_table_style(ps_session *s, unsigned flags) {
    unsigned layout = PS_TABLE_CATEGORY_COLUMN|PS_TABLE_COMPACT_SECTIONS|PS_TABLE_TABS_INSIDE|PS_TABLE_PLAIN_KEYS|PS_TABLE_TAB_BRACKETS|PS_TABLE_PROMINENT_KEYS|PS_TABLE_COMPACT_FOOTER|PS_TABLE_COMPACT_SCROLLBAR|PS_TABLE_SECTION_BOTTOM;
    if (s->editing)
        flags = (flags & ~layout) | (s->table_style & layout);
    if ((s->table_style ^ flags) & layout)
        s->control_header = s->scroll = 0;
    s->canvas_kind=0;
    s->table_style = flags & (PS_TABLE_GRID | PS_TABLE_RED_BORDER | PS_TABLE_TITLE_BAR | layout);
    s->dirty = 1;
}
PS_API unsigned ps_alignof_session(void) {
    struct A {
        char c;
        ps_session s;
    };
    return (unsigned)(sizeof(struct A) - sizeof(ps_session));
}
PS_API ps_session *ps_init(void *memory, unsigned bytes, const ps_config *c) {
    ps_session *s = memory;
    unsigned i;
    if (!s || !c || !c->write || bytes < sizeof *s)
        return 0;
    zero(s, sizeof *s);
    s->history_limit = PS_HISTORY;
    s->io = *c;
    s->table_style = PS_TABLE_DEFAULT;
    s->menu_style=PS_MENU_TABLE|PS_MENU_NUMBERS;s->document_style=PS_EDITOR_HEADER;
    s->theme = PS_THEME_DEFAULT;
#ifdef PS_ASCII_ONLY
    s->io.flags &= ~(PS_COLOR | PS_UNICODE);
#endif
    s->io.columns = c->columns ? (c->columns < 8 ? 8 : min(c->columns, 240)) : 80;
    s->io.rows = c->rows ? (c->rows < 4 ? 4 : min(c->rows, 80)) : 24;
    s->next_id = 1;
    ps_telnet_init(&s->telnet, PS_TN_CONTROLS, s->io.columns, s->io.rows);
    str(s->telnet.terminal, "byte stream", sizeof s->telnet.terminal);
    if (!(c->flags & PS_NO_BUILTINS))
        for (i = 0; i < sizeof builtins / sizeof builtins[0]; ++i)
            ps_register(s, &builtins[i]);
    return s;
}
static void telnet_write(void *context, const unsigned char *bytes, size_t count) {
    ps_session *s = (ps_session *)context;
    s->io.write(s->io.context, bytes, (unsigned)count);
}
PS_API void ps_start(ps_session *s) {
    if (s->started)
        return;
    s->started = 1;
    if (s->io.flags & PS_TELNET)
        ps_telnet_greet(&s->telnet, telnet_write, s);
    if (s->io.flags & PS_INTERACTIVE) {
        ansi(s, "\033[?2004h");
        prompt(s);
    }
}
static unsigned visible(ps_session *s, unsigned *indices) {
    unsigned i, n = 0;
    for (i = 0; i < s->task_count; ++i)
        if (contains(s->tasks[i].title, s->filter))
            indices[n++] = i;
    return n;
}
static unsigned tab_count(ps_session *s) {
    unsigned ids[PS_ROWS];
    if (s->control_view && s->panel)
        return s->panel->pages[s->tab].count;
    if (s->tab == 0)
        return visible(s, ids);
    if (s->tab == 1)
        return s->var_count;
    if (s->tab == 2)
        return s->job.total ? 1 : 0;
    return 10;
}
typedef struct {
    const ps_column *columns;
    unsigned count, widths[3];
} Grid;
static unsigned table_gutter(const ps_session *s) {
    return s->io.columns>=64 && !(s->table_style&PS_TABLE_COMPACT_SCROLLBAR)?2:0;
}
static Grid grid_for(ps_session *s) {
    static const ps_column tasks[] = {{"ID", 4, 4, 0}, {"State", 8, 8, 0}, {"Task", 12, 0, 1}};
    static const ps_column pairs[] = {{"Key", 10, 24, 1}, {"Value", 14, 0, 3}};
    static const ps_column jobs[] = {
        {"Job", 10, 24, 1}, {"Progress", 12, 0, 3}, {"State", 8, 8, 0}};
    static const ps_column stats[] = {{"Metric", 10, 24, 1}, {"Value", 14, 0, 3}};
    static const ps_column controls[] = {
        {"Category", 10, 16, 1}, {"Control", 14, 26, 1}, {"Value", 16, 0, 3}};
    static const ps_column compact[] = {{"Control", 12, 20, 1}, {"Value", 16, 0, 3}};
    Grid g;
    g.columns = s->tab == 0 ? tasks : s->tab == 1 ? pairs : s->tab == 2 ? jobs : stats;
    g.count = (s->tab == 0 || s->tab == 2) ? 3 : 2;
    if (s->control_view) {
        unsigned category = (!PS_CONTROL_SECTION_ROWS || (s->table_style & PS_TABLE_CATEGORY_COLUMN)) && s->io.columns >= 64;
        g.columns = category ? controls : compact;
        g.count = category ? 3 : 2;
    }
    ps_layout_columns(s->io.columns - 2 - table_gutter(s), g.columns, g.count, g.widths);
    return g;
}
static void grid_vertical(ps_session *s) {
    ink(s, 7);
    out(s, (s->io.flags & PS_UNICODE) ? "│" : "|");
}
static unsigned table_slots(const ps_session *s) {
    unsigned reserved=13+((s->table_style&PS_TABLE_TABS_INSIDE)&&s->io.rows>=15?1:0);
    return s->io.rows>reserved?s->io.rows-reserved:1;
}
static void grid_divide(ps_session *s) {
    out(s, " ");
    if (s->table_style & PS_TABLE_GRID)
        grid_vertical(s);
    else
        out(s, " ");
    out(s, " ");
}
static void grid_text(ps_session *s, const char *text, unsigned width_) {
    unsigned suffix = (s->io.flags & PS_UNICODE) ? 1 : 3;
    if (cells(text, length(text), s->io.flags) > width_ && width_ >= suffix) {
        field(s, text, width_ - suffix);
        out(s, (s->io.flags & PS_UNICODE) ? "…" : "...");
    } else
        field(s, text, width_);
}
static void grid_border(ps_session *s, const Grid *g, unsigned kind, int joined) {
    unsigned i, j;
    const char *left = kind == 0 ? "╭" : kind == 2 ? "╰" : "├";
    const char *right = kind == 0 ? "╮" : kind == 2 ? "╯" : "┤";
    const char *middle = kind == 0 || kind == 4 ? "┬" : (kind == 2 || kind == 3) ? "┴" : "┼";
    ink(s, 7);
    out(s, (s->io.flags & PS_UNICODE) ? left : "+");
    for (i = 0; i < g->count; ++i) {
        for (j = 0; j < g->widths[i] + 2; ++j)
            out(s, (s->io.flags & PS_UNICODE) ? "─" : "-");
        if (i + 1 < g->count)
            out(s, (s->io.flags & PS_UNICODE)
                       ? ((joined && (s->table_style & PS_TABLE_GRID)) ? middle : "─")
                       : ((joined && (s->table_style & PS_TABLE_GRID)) ? "+" : "-"));
    }
    out(s, (s->io.flags & PS_UNICODE) ? right : "+");
    reset(s);
}
static void table_open(ps_session *s, unsigned tab) {
    if(s->document_open||ps_form_active(s))return;
    ps_file_picker_close(s);
    ps_menu_close(s);
    s->tab = tab;
    s->selected = s->scroll = 0;
    s->editing = 0;
    if (!(s->io.flags & PS_INTERACTIVE)) {
        notice(s, "interactive terminal required");
        return;
    }
    if (!s->board) {
        s->board = 1;
        reset(s);
        s->canvas_kind=0;
        ansi(s, "\033[?1049h\033[?25l");
    if(PS_MOUSE)ansi(s,"\033[?1002h\033[?1006h");
    }
    s->dirty = 1;
}
static void table_close(ps_session *s) {
    if(PS_MOUSE)ansi(s,"\033[?1002l\033[?1006l");
    s->mouse_drag=0;
    s->editing = 0;
    s->dialog = s->control_view = 0;
    s->board = 0;
    s->filter[0] = 0;
    ansi(s, "\033[0m\033[?25h\033[?1049l");
    prompt(s);
}
static void edit_insert(char *, unsigned *, unsigned *, unsigned, unsigned);
static void edit_delete(char *, unsigned *, unsigned *, int);
#include "portable_menu.inc"
#include "portable_editor.inc"
#include "portable_files.inc"
static void enter_edit(ps_session *, unsigned, const char *);
static void edit_insert(char *, unsigned *, unsigned *, unsigned, unsigned);
static void edit_delete(char *, unsigned *, unsigned *, int);
#include "portable_controls.inc"
#include "portable_dashboard.inc"
static void task_row(ps_session *s, unsigned index, const Grid *g) {
    Task *t = &s->tasks[index];
    char b[20];
    ink(s, 1);
    str(b, t->selected ? ((s->io.flags & PS_UNICODE) ? "◆ " : "* ") : "  ", sizeof b);
    dec(b + length(b), t->id);
    field(s, b, g->widths[0]);
    grid_divide(s);
    ink(s, t->done ? 5 : 6);
    field(s,
          (s->io.flags & PS_UNICODE) ? (t->done ? "✓ Done" : "○ Open")
                                     : (t->done ? "[x] Done" : "[ ] Open"),
          g->widths[1]);
    grid_divide(s);
    ink(s, t->done ? 0 : 4);
    grid_text(s, t->title, g->widths[2]);
}
static void pair_row(ps_session *s, const char *key, const char *value, const Grid *g) {
    ink(s, 1);
    grid_text(s, key, g->widths[0]);
    grid_divide(s);
    ink(s, s->theme==PS_THEME_DEFAULT?PS_INK_STRING:2);
    grid_text(s, value, g->widths[1]);
}
static void stat_row(ps_session *s, unsigned i, const Grid *g) {
    char b[32];
    const char *key = "", *value = b;
    b[0] = 0;
    switch (i) {
    case 0:
        key = "Core";
        value = "PortableShell " PS_VERSION;
        break;
    case 1:
        key = "Session bytes";
        dec(b, sizeof *s);
        break;
    case 2:
        key = "Heap bytes";
        value = "0";
        break;
    case 3:
        key = "Input bytes";
        dec(b, s->bytes_in);
        break;
    case 4:
        key = "Commands run";
        dec(b, s->executions);
        break;
    case 5:
        key = "History entries";
        dec(b, s->history_count);
        break;
    case 6:
        key = "Transport";
        value = (s->io.flags & PS_TELNET) ? "Telnet / byte callbacks" : "Raw / byte callbacks";
        break;
    case 7:
        key = "Terminal";
        value = s->telnet.terminal;
        break;
    case 8:
        key = "Uptime seconds";
        dec(b, s->now / 1000);
        break;
    case 9:
        key = "Memory backend";
        value = ps_memory_backend();
        break;
    }
    pair_row(s, key, value, g);
}
static void progress_bar(ps_session *s, unsigned width_) {
    unsigned i, pct = s->job.total ? (unsigned)((s->job.done * 100u) / s->job.total) : 0;
    ink(s, 1);
    for (i = 0; i < width_; ++i)
        out(s, i * 100 / width_ < pct ? ((s->io.flags & PS_UNICODE) ? "━" : "#")
                                      : ((s->io.flags & PS_UNICODE) ? "─" : "."));
    out(s, " ");
    ps_number(s, pct);
    out(s, "%");
}
static void render(ps_session *s) {
    unsigned gutter=table_gutter(s);
    unsigned w = s->io.columns-gutter, h = s->io.rows, i, count, slots, ids[PS_ROWS], inner;
    unsigned inside = !!(s->table_style & PS_TABLE_TABS_INSIDE) && h>=15;
    unsigned body = 7+inside, tabUsed = 1;
    unsigned displayCount, focus;
    unsigned short controlItems[512];
    Grid grid;
    static const char *const tabs[] = {"Tasks", "Variables", "Jobs", "Session"};
    if (!s->board || !(s->io.flags & PS_INTERACTIVE))
        return;
    s->dirty = 0;
#if PS_DASHBOARD
    if(s->control_view&&s->dashboard&&w>=42&&h>=14){dashboard_render(s);return;}
#endif
    s->mouse_edit_width=0;
    zero(s->mouse_tab_left,sizeof s->mouse_tab_left);
    zero(s->mouse_tab_right,sizeof s->mouse_tab_right);
    s->last_paint = s->now;
    canvas_begin(s,4,0);
    reset(s);
    ansi(s, "\033[?25l");
    if (w < 42 || h < 14) {
        at(s, 1, 1);
        ansi(s, "\033[J");
        ink(s, 1);
        out(s, "PortableShell  ");
        ps_number(s, s->io.columns);
        out(s, (s->io.flags & PS_UNICODE) ? " \xc3\x97 " : " x ");
        ps_number(s, h);
        at(s, 3, 1);
        out(s, (s->io.flags & PS_UNICODE) ? "42 \xc3\x97 14 minimum | Esc shell" : "42 x 14 minimum | Esc shell");
        frame_end(s);
        return;
    }
    inner = w - 6;
    grid = grid_for(s);
    count = tab_count(s);
    slots = table_slots(s);
    if (s->selected >= count)
        s->selected = count ? count - 1 : 0;
    focus = s->selected;
    displayCount = s->control_view ? control_items(s,controlItems,&focus,1) : count;
    if (s->scroll > focus)
        s->scroll = focus;
    if (focus >= s->scroll + slots)
        s->scroll = focus - slots + 1;
    s->scroll = min(s->scroll,displayCount>slots?displayCount-slots:0);
    at(s, 1, 1);
    ansi(s, "\033[2K");
    out(s, "  ");
    if (s->table_style & PS_TABLE_TITLE_BAR) {
        surface(s, 1);
        out(s, " ");
    } else
        ink(s, 1);
    out(s, (s->io.flags & PS_UNICODE) ? "◆ " : "+ ");
    out(s, "PortableShell");
    if (s->table_style & PS_TABLE_TITLE_BAR)
        out(s, " ");
    reset(s);
    ink(s, 0);
    out(s, (s->io.flags & PS_UNICODE) ? "  ·  " : "  /  ");
    ink(s, 4);
    if (s->control_view)
        grid_text(s, s->panel->title, min(24, w - 24 - (w >= 60 ? 14 : 0)));
    else
        ps_write(s, tabs[s->tab]);
    ink(s, 0);
    if (w >= 60) {
        out(s, "   ");
        ps_number(s, count);
        out(s, count == 1 ? " entry" : " entries");
    }
    if (!s->control_view && s->tab == 0 && w >= 76) {
        unsigned done = 0;
        for (i = 0; i < s->task_count; ++i)
            done += s->tasks[i].done != 0;
        at(s, 1, w - 21);
        ink(s, 5);
        ps_number(s, done);
        out(s, "/");
        ps_number(s, s->task_count);
        out(s, " complete");
    }
    reset(s);
    at(s, 3+inside, 1);
    ansi(s, "\033[2K");
    if(inside){out(s," ");grid_vertical(s);out(s," ");}else out(s,"  ");
    for (i = 0; i < (s->control_view ? s->panel->count : 4); ++i) {
        const char *title = s->control_view ? s->panel->pages[i].title : w<60?(i==1?"Vars":i==3?"Info":tabs[i]):tabs[i];
        unsigned titleWidth=s->control_view?min(16,w>=80?(w-3)/s->panel->count-6:w-12):length(title);
        unsigned segment=titleWidth+5+((i==s->tab&&(!(s->io.flags&PS_COLOR)||PS_TAB_BRACKETS||(s->table_style&PS_TABLE_TAB_BRACKETS)))?2:0);
        if (((s->control_view && w < 80) || (inside && w<60)) && i != s->tab)
            continue;
        if(inside&&tabUsed+segment>w-4)continue;
        s->mouse_tab_left[i]=tabUsed+2+(inside?1:0);
        s->mouse_tab_right[i]=s->mouse_tab_left[i]+segment-1;
        if (i == s->tab) {
            surface(s, 1);
            if (!(s->io.flags & PS_COLOR)||PS_TAB_BRACKETS||(s->table_style&PS_TABLE_TAB_BRACKETS))
                out(s, "[");
        } else
            ink(s, 0);
        out(s, " ");
        ps_number(s, i + 1);
        out(s, " ");
        grid_text(s,title,titleWidth);
        out(s, " ");
        if (i == s->tab && (!(s->io.flags & PS_COLOR)||PS_TAB_BRACKETS||(s->table_style&PS_TABLE_TAB_BRACKETS)))
            out(s, "]");
        reset(s);
        out(s, " ");
        tabUsed+=segment;
    }
    if(inside){spaces(s,w-4-tabUsed);grid_vertical(s);reset(s);}
    at(s, 2, 1);
    ansi(s, "\033[2K");
    if (s->control_view && !control_sections(s) && count && grid.count == 2) {
        out(s, "  ");
        ink(s, 0);
        grid_text(s, s->panel->pages[s->tab].controls[s->selected].group, w - 4);
        reset(s);
    }
    at(s, inside?3:4, 2);
    grid_border(s, &grid, 0, !inside);
    if(inside){at(s,5,2);grid_border(s,&grid,4,1);}
    at(s, 5+inside, 2);
    ink(s, 7);
    out(s, (s->io.flags & PS_UNICODE) ? "│" : "|");
    out(s, " ");
    for (i = 0; i < grid.count; ++i) {
        ink(s, 6);
        field(s, grid.columns[i].title, grid.widths[i]);
        if (i + 1 < grid.count)
            grid_divide(s);
    }
    ink(s, 7);
    out(s, " ");
    out(s, (s->io.flags & PS_UNICODE) ? "│" : "|");
    reset(s);
    at(s, 6+inside, 2);
    grid_border(s, &grid, s->control_view && s->scroll<displayCount && (controlItems[s->scroll]&0xe000) ? 3 : 1, 1);
    if (!s->control_view && s->tab == 0)
        visible(s, ids);
    for (i = 0; i < slots; ++i) {
        unsigned idx = s->scroll + i;
        unsigned heading = 0, rule = 0;
        if (s->control_view && idx < displayCount) {
            heading = controlItems[idx] & 0x8000;
            rule = controlItems[idx] & 0x6000;
            idx = controlItems[idx] & 0x1fff;
        } else if (s->control_view) idx = count;
        at(s, body + i, 2);
        if (rule) {
            unsigned line = s->scroll+i;
            unsigned joined = rule == 0x4000 ? (i && !(controlItems[line-1]&0xe000))
                : (i+1<slots && (line+1>=displayCount || !(controlItems[line+1]&0xe000)));
            grid_border(s,&grid,rule==0x4000?3:4,joined);continue;
        }
        ink(s, 7);
        out(s, (s->io.flags & PS_UNICODE) ? "│" : "|");
        if (heading) {
            surface(s, s->control_header && s->selected == idx ? 1 : 0);
            if (!(s->io.flags & PS_COLOR) && s->control_header && s->selected == idx) ansi(s,"\033[7m");
            out(s," ");control_section_row(s,idx,inner);out(s," ");
            reset(s);grid_vertical(s);reset(s);continue;
        }
        out(s, " ");
        if (idx < count) {
            if (idx == s->selected && (!s->control_view || !s->control_header)) {
                if (s->io.flags & PS_COLOR)
                    surface(s, 2);
                else
                    ansi(s, "\033[7m");
            }
            if (s->control_view) {
                if (idx == s->selected) s->control_edit_row = body+i;
                control_row(s, idx, &grid);
            } else if (s->tab == 0)
                task_row(s, ids[idx], &grid);
            else if (s->tab == 1)
                pair_row(s, s->vars[idx].key, s->vars[idx].value, &grid);
            else if (s->tab == 3)
                stat_row(s, idx, &grid);
            else {
                ink(s, 4);
                unsigned pct = s->job.total ? s->job.done * 100 / s->job.total : 0;
                grid_text(s, s->job.name, grid.widths[0]);
                grid_divide(s);
                progress_bar(s, grid.widths[1] - 5);
                spaces(s, pct >= 100 ? 0 : pct >= 10 ? 1 : 2);
                grid_divide(s);
                ink(s, s->job.cancelled ? 3 : s->job.done < s->job.total ? 2 : 5);
                field(s,
                      s->job.cancelled             ? "Stopped"
                      : s->job.done < s->job.total ? "Running"
                                                   : "Done",
                      grid.widths[2]);
            }
            reset(s);
        } else {
            unsigned j;
            for (j = 0; j < grid.count; ++j) {
                spaces(s, grid.widths[j]);
                if (j + 1 < grid.count)
                    grid_divide(s);
            }
        }
        ink(s, 7);
        out(s, " ");
        out(s, (s->io.flags & PS_UNICODE) ? "│" : "|");
        reset(s);
    }
    if(gutter)for(i=0;i<slots;++i){at(s,body+i,w);reset(s);spaces(s,gutter);}
    if (displayCount > slots) {
        unsigned thumb = slots * slots / displayCount;
        if (!thumb) thumb=1;
        unsigned top = s->scroll * (slots-thumb) / (displayCount-slots);
        for(i=0;i<slots;++i){
            unsigned active = i>=top && i<top+thumb;
            unsigned rule = s->control_view && s->scroll+i<displayCount && (controlItems[s->scroll+i]&0x6000);
            at(s,body+i,w-1+gutter);ink(s,active?1:7);
            out(s,(s->io.flags&PS_UNICODE)?(!gutter&&rule?(active?"┫":"┤"):(active?"┃":"│")):(active?"#":!gutter&&rule?"+":"|"));
        }
        reset(s);
    }
    at(s, h - 6, 2);
    grid_border(s, &grid, 3, !(s->control_view && s->scroll+slots-1<displayCount && (controlItems[s->scroll+slots-1]&0xe000)));
    at(s, h - 5, 2);
    grid_vertical(s);
    spaces(s, w - 4);
    grid_vertical(s);
    at(s, h - 5, 4);
    ink(s, 0);
    if (s->control_view && s->control_header) {
        out(s,"Section: ");grid_text(s,s->panel->pages[s->tab].controls[s->selected].group,w-15);
    } else {
        out(s, "Row ");ps_number(s, count ? s->selected + 1 : 0);
        out(s, "/");ps_number(s, count);
    }
    if (!s->control_view && s->filter[0] && s->tab == 0)
        out(s, " filtered");
    if (!s->control_view && s->tab == 0) {
        unsigned selected = 0;
        for (i = 0; i < s->task_count; ++i)
            selected += s->tasks[i].selected != 0;
        at(s, h - 5, w - 18);
        ink(s, 1);
        ps_number(s, selected);
        out(s, " selected");
    }
    reset(s);
    at(s, h - 4, 2);
    grid_border(s, &grid, 2, 0);
    at(s, h - 3, 1);
    ansi(s, "\033[2K");
    out(s, "  ");
    if (s->editing && !s->control_view &&
        !(PS_WORKSPACE_INLINE_EDIT && (s->editing == 2 || s->editing == 3))) {
        const char *label = s->editing == 1   ? "New task"
                            : s->editing == 2 ? "Edit task"
                            : s->editing == 3 ? "Value"
                                              : "Filter";
        ink(s, 1);
        out(s, label);
        out(s, "  ");
        ink(s, 4);
        field(s, s->edit, w - length(label) - 6);
        reset(s);
        at(s, h - 3, 5 + length(label) + cells(s->edit, s->editpos, s->io.flags));
        ansi(s, "\033[?25h");
    } else {
        ink(s, 5);
        if(s->theme==PS_THEME_DEFAULT)ink(s,PS_INK_TEXT);
        field(s, s->notice, w - 4);
        reset(s);
    }
    at(s, h - 1, 1);
    ansi(s, "\033[2K");
    out(s, "  ");
    ink(s, 0);
    footer_field(s,
          s->control_view ? control_hint(s)
          : s->editing    ? "Enter save   Esc cancel"
          : s->tab == 0   ? (w < 60 ? "n new  Enter edit  d done  / filter"
                                    : "n new   Enter edit   d done   Space select   / filter")
          : s->tab == 1   ? "Enter edit value   Del remove"
          : s->tab == 2   ? "c cancel job"
                          : "Live session counters",
          w - 4,0);
    reset(s);
    at(s, h, 1);
    ansi(s, "\033[2K");
    reset(s);
    ink(s,0);
    out(s, "  ");
    footer_field(
        s,
        s->control_view && s->editing ? "Draft   Esc cancel"
        : s->editing ? ((s->io.flags & PS_UNICODE) ? "←/→ cursor  Home/End  ^U clear"
                                                   : "Left/Right cursor  Home/End  ^U clear")
        : s->control_view ? ((s->io.flags & PS_UNICODE) ? "↑/↓ rows  Tab pages  Esc shell"
                                                        : "Up/Dn rows  Tab pages  Esc shell")
        : (s->io.flags & PS_UNICODE)
            ? (w < 60 ? "↑↓ move   Tab tabs   Esc shell" : "↑/↓ move   ←/→ tabs   Esc shell")
            : (w < 60 ? "Up/Dn move  Tab tabs  Esc shell" : "Up/Down move   Tab tabs   Esc shell"),
        w - 3,1);
    reset(s);
    if (w >= 76) {
        at(s, h, w - 23);
        surface(s, 0);
        ps_number(s, s->io.columns);
        out(s, (s->io.flags & PS_UNICODE) ? "×" : "x");
        ps_number(s, h);
        out(s, "  ");
        if (s->control_view)
            grid_text(s, s->panel->status, 15);
        else
            out(s, (s->io.flags & PS_TELNET) ? "Telnet" : "byte stream");
        reset(s);
    }
    if (s->control_view)
        control_overlay(s, &grid);
    else if (PS_WORKSPACE_INLINE_EDIT && (s->editing == 2 || s->editing == 3) && count) {
        unsigned x = 4, start = 0, width = grid.widths[grid.count - 1];
        for (i = 0; i + 1 < grid.count; ++i)
            x += grid.widths[i] + 3;
        while (cells(s->edit + start, s->editpos - start, s->io.flags) >= width)
            start = next(s->edit, start);
        s->mouse_edit_x=x;s->mouse_edit_y=body+s->selected-s->scroll;
        s->mouse_edit_start=start;s->mouse_edit_width=width;
        at(s, body + s->selected - s->scroll, x);
        if (s->io.flags & PS_COLOR)
            surface(s, 1);
        else
            ansi(s, "\033[7m");
        field(s, s->edit + start, width);
        reset(s);
        at(s, body + s->selected - s->scroll,
           x + cells(s->edit + start, s->editpos - start, s->io.flags));
        ansi(s, "\033[?25h");
    } else if (s->editing) {
        const char *label = s->editing == 1   ? "New task"
                            : s->editing == 2 ? "Edit task"
                            : s->editing == 3 ? "Value"
                                              : "Filter";
        at(s, h - 3, min(w - 1, 5 + length(label) + cells(s->edit, s->editpos, s->io.flags)));
    } else {
        at(s, 3, 1);
    }
    (void)inner;
    frame_end(s);
}
static unsigned collect_completions(ps_session *, unsigned *, unsigned *, const char **);
static unsigned encode_completion(const char *, unsigned, char *);
/* Append-only preview: never rewrite a user's quoting or run a command. */
static const char *suggest(ps_session *s, char *buffer) {
    unsigned i, start, end, count, n;
    const char *matches[PS_COMMANDS];
    if (!(s->io.flags & PS_INTERACTIVE) || (s->io.flags & PS_NO_SUGGESTIONS) ||
        !s->len || s->pos != s->len || s->overflow || s->search || s->completion)
        return 0;
    for (i = s->history_count; i; --i) {
        const char *p = ps_history_get(s, i - 1);
        if (prefix(p, s->line) && length(p) > s->len)
            return p;
    }
    count = collect_completions(s, &start, &end, matches);
    /* An empty token is deliberate; don't fill arbitrary arguments after a space. */
    if (start == s->len)
        return 0;
    for (i = 0; i < count; ++i) {
        char encoded[PS_LINE];
        n = encode_completion(matches[i], length(matches[i]), encoded);
        if (n <= s->len - start || start + n >= PS_LINE ||
            !name_match(s, encoded, s->line + start, 1))
            continue;
        copy(buffer, s->line, s->len);
        copy(buffer + s->len, encoded + s->len - start, start + n - s->len + 1);
        return buffer;
    }
    return 0;
}
#if PS_FORM
static void form_render(ps_session *s);
#endif
static void prompt(ps_session *s) {
#if PS_FORM
    if(s->form){form_render(s);return;}
#endif
    char display[PS_LINE>256?PS_LINE:256];
    unsigned i,last=0,lines=0,shown,skip,old;
    for(i=0;s->custom_prompt&&s->prompt_text[i];++i)
        if(s->prompt_text[i]=='\n'){last=i+1;++lines;}
    shown=min(lines,s->io.rows>2?s->io.rows-2:0);skip=lines-shown;
    unsigned prompt_width=s->custom_prompt
        ? min(cells(s->prompt_text+last,length(s->prompt_text+last),s->io.flags),s->io.columns-3)
        : 20+(s->task_count>=10?1:0);
    unsigned start = 0,
             avail = s->io.columns > prompt_width + 2 ? s->io.columns - prompt_width - 2 : 8;
    const char *hint;
    if (!(s->io.flags & PS_INTERACTIVE) || s->board || s->menu_depth || s->file_count || s->document_open || s->exit)
        return;
    frame_begin(s);
    ansi(s, "\r");
    old=min(s->prompt_rows_drawn,s->io.rows-1);
    if(old){ansi(s,"\033[");ps_number(s,old);out(s,"A");}
    if(old>shown){
        for(i=0;i<=old;++i){ansi(s,"\033[K");if(i<old)ansi(s,"\033[B");}
        ansi(s,"\033[");ps_number(s,old);out(s,"A");
    }
    s->prompt_rows_drawn=shown;
    if (s->custom_prompt) {
        unsigned part,pos=0,line=0,column=0;
        for(part=0;part<s->prompt_part_count;++part){
            style(s,s->prompt_parts[part].style);
            while(pos<s->prompt_parts[part].end){
                unsigned start=pos,n,w;
                while(pos<s->prompt_parts[part].end&&s->prompt_text[pos]!='\n')++pos;
                n=pos-start;
                if(line>=skip){
                    copy(display,s->prompt_text+start,n);display[n]=0;
                    w=min(cells(display,n,s->io.flags),s->io.columns-3-column);
                    field(s,display,w);column+=w;
                }
                if(pos<s->prompt_parts[part].end){
                    if(line>=skip){reset(s);ansi(s,"\033[K");out(s,"\r\n");style(s,s->prompt_parts[part].style);}
                    ++line;++pos;column=0;
                }
            }
        }
    } else {
        ink(s, 1);
        out(s, "PortableShell");
        ink(s, 0);
        out(s, " [");
        ps_number(s, s->task_count);
        out(s, "] ");
        ink(s, 5);
        out(s, "> ");
    }
    reset(s);
    while (cells(s->line + start, s->pos - start, s->io.flags) > avail)
        start = next(s->line, start);
    str(display, s->line, sizeof display);
    if (s->uppercase_echo)
        for (i = 0; display[i]; ++i)
            if (display[i] >= 'a' && display[i] <= 'z')
                display[i] -= 32;
    field(s, display + start, min(cells(display + start, s->len - start, s->io.flags), avail));
    hint = suggest(s, s->suggestion);
    if (hint) str(s->suggestion, hint, sizeof s->suggestion);
    else ps_secure_clear(s->suggestion, sizeof s->suggestion);
    if (hint && s->pos == s->len && cells(s->line + start, s->len - start, s->io.flags) < avail) {
        ink(s, 0);
        field(s, hint + s->len,
              min(cells(hint + s->len, length(hint + s->len), s->io.flags),
                  avail - cells(s->line + start, s->len - start, s->io.flags)));
        reset(s);
    } else
        ps_secure_clear(s->suggestion, sizeof s->suggestion);
    ansi(s, "\033[K\r\033[");
    ps_number(s, prompt_width + cells(s->line + start, s->pos - start, s->io.flags) + 1);
    out(s, "G");
    if (s->search) {
        ansi(s, "\r");
        ink(s, 1);
        out(s, "search: ");
        ps_write(s, s->query);
        ink(s, 0);
        out(s, "  ");
        ps_write(s, s->line);
        reset(s);
        ansi(s,"\033[K");
    }
    frame_end(s);
}
static void edit_insert(char *buf, unsigned *len, unsigned *pos, unsigned cap, unsigned c) {
    if (*len + 1 >= cap)
        return;
    move(buf + *pos + 1, buf + *pos, *len - *pos + 1);
    buf[(*pos)++] = (char)c;
    ++*len;
}
static void edit_delete(char *buf, unsigned *len, unsigned *pos, int back) {
    unsigned a = *pos, b;
    if (back) {
        if (!a)
            return;
        a = prev(buf, a);
        b = *pos;
        *pos = a;
    } else {
        if (a == *len)
            return;
        b = next(buf, a);
    }
    move(buf + a, buf + b, *len - b + 1);
    *len -= b - a;
}
static void line_set(ps_session *s, const char *p) {
    str(s->line, p, PS_LINE);
    s->pos = s->len = length(s->line);
}
static void enter_edit(ps_session *s, unsigned mode, const char *p) {
    s->editing = mode;
    s->notice[0] = 0;
    str(s->edit, p, PS_VALUE);
    s->editpos = s->editlen = length(s->edit);
    s->dirty = 1;
}
static int add_task(ps_session *s, const char *p) {
    Task *t;
    if (!*p || length(p) >= PS_VALUE || s->task_count == PS_ROWS)
        return 0;
    t = &s->tasks[s->task_count++];
    zero(t, sizeof *t);
    str(t->title, p, PS_VALUE);
    t->id = s->next_id++;
    return 1;
}
static void table_key(ps_session *s, unsigned key) {
    unsigned n = tab_count(s), ids[PS_ROWS], idx = 0, i, any = 0;
    if (s->control_view) {
        control_key(s, key);
        return;
    }
    if (s->tab == 0) {
        visible(s, ids);
        if (n)
            idx = ids[s->selected < n ? s->selected : 0];
    }
    if (s->editing) {
        if (key == 27 || key == 3) {
            s->editing = 0;
            notice(s, "Cancelled");
        } else if (key == 13) {
            int ok = 1;
            if (s->editing == 1)
                ok = add_task(s, s->edit);
            else if (s->editing == 2 && n)
                str(s->tasks[idx].title, s->edit, PS_VALUE);
            else if (s->editing == 3 && n)
                str(s->vars[s->selected].value, s->edit, PS_VALUE);
            else if (s->editing == 4)
                str(s->filter, s->edit, sizeof s->filter);
            s->editing = 0;
            notice(s, ok ? "Saved" : "Task empty or capacity reached");
        } else if (key == PS_KEY_LEFT)
            s->editpos = prev(s->edit, s->editpos);
        else if (key == PS_KEY_RIGHT)
            s->editpos = next(s->edit, s->editpos);
        else if (key == PS_KEY_HOME || key == 1)
            s->editpos = 0;
        else if (key == PS_KEY_END || key == 5)
            s->editpos = s->editlen;
        else if (key == 21) {
            s->edit[0] = 0;
            s->editpos = s->editlen = 0;
        } else if (key == 8 || key == 127)
            edit_delete(s->edit, &s->editlen, &s->editpos, 1);
        else if (key == PS_KEY_DELETE)
            edit_delete(s->edit, &s->editlen, &s->editpos, 0);
        else if (key >= 32 && key < 256)
            edit_insert(s->edit, &s->editlen, &s->editpos, PS_VALUE, key);
        s->dirty = 1;
        return;
    }
    if (key == 27 || key == 3 || key == 'q') {
        table_close(s);
        return;
    }
    if (key == 9 || key == PS_KEY_RIGHT) {
        s->tab = (s->tab + 1) % 4;
        s->selected = s->scroll = 0;
    } else if (key == PS_KEY_BACKTAB || key == PS_KEY_LEFT) {
        s->tab = (s->tab + 3) % 4;
        s->selected = s->scroll = 0;
    } else if (key >= '1' && key <= '4') {
        s->tab = key - '1';
        s->selected = s->scroll = 0;
    } else if (key == PS_KEY_DOWN || key == 'j') {
        if (s->selected + 1 < n)
            ++s->selected;
    } else if (key == PS_KEY_UP || key == 'k') {
        if (s->selected)
            --s->selected;
    } else if (key == PS_KEY_HOME)
        s->selected = 0;
    else if (key == PS_KEY_END)
        s->selected = n ? n - 1 : 0;
    else if (key == PS_KEY_PAGEDOWN)
        s->selected = min(s->selected + table_slots(s), n ? n - 1 : 0);
    else if (key == PS_KEY_PAGEUP)
        s->selected = s->selected > table_slots(s)
                          ? s->selected - table_slots(s)
                          : 0;
    else if (s->tab == 0) {
        if (key == 'n')
            enter_edit(s, 1, "");
        else if (key == '/')
            enter_edit(s, 4, s->filter);
        else if (key == 13 && n)
            enter_edit(s, 2, s->tasks[idx].title);
        else if (key == ' ' && n)
            s->tasks[idx].selected ^= 1;
        else if (key == 'd' && n) {
            for (i = 0; i < s->task_count; ++i)
                if (s->tasks[i].selected) {
                    s->tasks[i].done ^= 1;
                    any = 1;
                }
            if (!any)
                s->tasks[idx].done ^= 1;
            notice(s, "Task state updated");
        } else if (key == PS_KEY_DELETE && n) {
            move(&s->tasks[idx], &s->tasks[idx + 1], (s->task_count - idx - 1) * sizeof(Task));
            --s->task_count;
            notice(s, "Task removed");
        }
    } else if (s->tab == 1 && n) {
        if (key == 13)
            enter_edit(s, 3, s->vars[s->selected].value);
        else if (key == PS_KEY_DELETE) {
            move(&s->vars[s->selected], &s->vars[s->selected + 1],
                 (s->var_count - s->selected - 1) * sizeof(Variable));
            --s->var_count;
            notice(s, "Variable removed");
        }
    } else if (s->tab == 2 && key == 'c') {
        s->job.active = 0;
        s->job.cancelled = 1;
        notice(s, "Job cancelled");
    }
    s->dirty = 1;
}
static unsigned encode_completion(const char *value, unsigned n, char *encoded) {
    unsigned i, count = 0, quoted = 0;
    for (i = 0; i < n; ++i)
        if (value[i] <= ' ' || value[i] == '\\' || value[i] == '"' || value[i] == '\'' || value[i] == '$' || value[i] == 96 ||
            value[i] == '|' || value[i] == '&' || value[i] == ';' || value[i] == '<' ||
            value[i] == '>' || value[i] == '#' || value[i] == '(' || value[i] == ')')
            quoted = 1;
    if (quoted)
        encoded[count++] = '"';
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)value[i];
        if (count + 4 >= PS_LINE)
            return 0;
        if (c == '"' || c == '\\' || c == '$' || c == 96)
            encoded[count++] = '\\';
        if (c == '\n' || c == '\t' || c == '\r') {
            encoded[count++] = '\\';
            c = c == '\n' ? 'n' : c == '\t' ? 't' : 'r';
        }
        encoded[count++] = (char)c;
    }
    if (quoted)
        encoded[count++] = '"';
    encoded[count] = 0;
    return count;
}
static int complete_replace(ps_session *s, const char *value, unsigned n, int finish) {
    char encoded[PS_LINE];
    unsigned count = encode_completion(value, n, encoded);
    if (n && !count)
        return 0;
    if (s->start + count + s->len - s->end + 2 >= PS_LINE)
        return 0;
    move(s->line + s->start + count, s->line + s->end, s->len - s->end + 1);
    copy(s->line + s->start, encoded, count);
    s->len = s->start + count + s->len - s->end;
    s->pos = s->end = s->start + count;
    if (finish && !s->line[s->pos])
        edit_insert(s->line, &s->len, &s->pos, PS_LINE, ' ');
    return 1;
}
static unsigned collect_completions(ps_session *s, unsigned *start, unsigned *end,
                                    const char **matches) {
    unsigned i, match_count;
    char token[PS_LINE];
    ps_args args;
    unsigned quote = 0, decoded = 0;
    (*start) = 0;
    for (i = 0; i < s->pos; ++i) {
        unsigned c = (unsigned char)s->line[i];
        if (c == '\\' && quote != '\'' && i + 1 < s->pos) {
            ++i;
            continue;
        }
        if (c == '\'' || c == '"') {
            if (!quote)
                quote = c;
            else if (quote == c)
                quote = 0;
        } else if (!quote && (c == ' ' || c == '\t'))
            (*start) = i + 1;
    }
    (*end) = s->pos;
    while (s->line[(*end)]) {
        unsigned c = (unsigned char)s->line[(*end)];
        if (!quote && (c == ' ' || c == '\t'))
            break;
        ++(*end);
        if (c == '\\' && quote != '\'' && s->line[(*end)]) {
            ++(*end);
            continue;
        }
        if (c == '\'' || c == '"') {
            if (!quote)
                quote = c;
            else if (quote == c)
                quote = 0;
        }
    }
    quote = 0;
    for (i = (*start); i < s->pos; ++i) {
        unsigned c = (unsigned char)s->line[i];
        if (c == '\'' || c == '"') {
            if (!quote) {
                quote = c;
                continue;
            }
            if (quote == c) {
                quote = 0;
                continue;
            }
        }
        if (c == '\\' && quote != '\'' && i + 1 < s->pos) {
            c = (unsigned char)s->line[++i];
            if (c == 'n')
                c = '\n';
            else if (c == 't')
                c = '\t';
            else if (c == 'r')
                c = '\r';
        }
        token[decoded++] = (char)c;
    }
    token[decoded] = 0;
    match_count = 0;
    if ((*start) == 0) {
        for (i = 0; i < s->command_count; ++i)
            if (name_match(s, s->commands[i].name, token, 1))
                matches[match_count++] = s->commands[i].name;
    } else {
        static const char *const task[] = {"add", "done", "remove", "rename", "list"};
        static const char *const board[] = {"tasks", "variables", "jobs", "session"};
        static const char *const controls[] = {"open", "list", "get", "set", "dashboard"};
        static const char *const theme[] = {"default", "cyan", "violet", "red", "onedark"};
        static const char *const display[] = {"color", "glyphs"}, *const colors[] = {"rgb", "ansi", "mono"}, *const glyphs[] = {"unicode", "ascii"};
        static const char *const txt[] = {"upper", "lower", "trim", "slug"};
        static const char *const hash_options[] = {"--rounds"};
        const char *const *opts = 0;
        unsigned count = 0;
        char partial[PS_LINE];
        copy(partial, s->line, (*start));
        partial[(*start)] = 0;
        ps_parse(partial, &args);
        if (args.argc) {
            for (i = 0; i < s->command_count; ++i)
                if (name_match(s, args.argv[0], s->commands[i].name, 0) &&
                    s->commands[i].complete) {
                    match_count =
                        min(PS_COMMANDS, s->commands[i].complete(s, args.argc,
                                                                 (const char *const *)args.argv,
                                                                 token, matches, PS_COMMANDS,
                                                                 s->commands[i].context));
                    break;
                }
            if (!match_count) {
                if (equal(args.argv[0], "task") && args.argc == 1) {
                    opts = task;
                    count = 5;
                } else if (equal(args.argv[0], "board")) {
                    opts = board;
                    count = 4;
                } else if (equal(args.argv[0], "controls") && args.argc == 1) {
                    opts = controls;
                    count = PS_DASHBOARD ? 5 : 4;
                } else if (equal(args.argv[0], "theme")) {
                    opts = theme;
                    count = 5;
                } else if (equal(args.argv[0], "display")) {
                    if(args.argc==1){opts=display;count=2;}
                    else if(equal(args.argv[1],"color")){opts=colors;count=3;}
                    else if(equal(args.argv[1],"glyphs")){opts=glyphs;count=2;}
                } else if (equal(args.argv[0], "text") && args.argc == 1) {
                    opts = txt;
                    count = 4;
                } else if (equal(args.argv[0], "hash") && args.argc >= 2) {
                    opts = hash_options;
                    count = 1;
                } else if (equal(args.argv[0], "get") || equal(args.argv[0], "set") ||
                           equal(args.argv[0], "unset")) {
                    for (i = 0; i < s->var_count && match_count < PS_COMMANDS; ++i)
                        if (prefix(s->vars[i].key, token))
                            matches[match_count++] = s->vars[i].key;
                } else if (equal(args.argv[0], "help")) {
                    for (i = 0; i < s->command_count; ++i)
                        if (name_match(s, s->commands[i].name, token, 1))
                            matches[match_count++] = s->commands[i].name;
                }
                for (i = 0; i < count; ++i)
                    if (prefix(opts[i], token))
                        matches[match_count++] = opts[i];
            }
        }
    }
    return match_count;
}
static void completion(ps_session *s) {
    unsigned i, j, common;
    if (!s->completion) {
        s->match_count = collect_completions(s, &s->start, &s->end, s->matches);
        if (!s->match_count) {
            ps_ring_bell(s);
            notice(s, "No completion");
            return;
        }
        common = length(s->matches[0]);
        for (i = 1; i < s->match_count; ++i) {
            for (j = 0; j < common && s->matches[0][j] == s->matches[i][j]; ++j) {
            }
            common = j;
        }
        complete_replace(s, s->matches[0], common, s->match_count == 1);
        if (s->match_count == 1) {
            return;
        }
        s->completion = 1;
        s->match_index = 0;
        ps_newline(s);
        for (i = 0; i < s->match_count; ++i) {
            ink(s, 1);
            ps_write(s, s->matches[i]);
            reset(s);
            out(s, "  ");
        }
        ps_newline(s);
    } else {
        unsigned n = length(s->matches[s->match_index]);
        complete_replace(s, s->matches[s->match_index], n, 0);
        s->match_index = (s->match_index + 1) % s->match_count;
    }
}
static void history_search(ps_session *s) {
    unsigned i = s->search_index;
    while (i) {
        const char *p = ps_history_get(s, --i);
        if (contains(p, s->query)) {
            line_set(s, p);
            s->search_index = i;
            return;
        }
    }
    ps_ring_bell(s);
    notice(s, "No older match");
}
#include "portable_form.inc"
static void shell_key(ps_session *s, unsigned key) {
    unsigned i;
    if (s->exit)
        return;
#if PS_FORM
    if(s->form){form_key(s,key);return;}
#endif
    if(s->file_count){file_key(s,key);return;}
    if(s->document_open){document_key(s,key);return;}
    if(s->menu_depth){menu_key(s,key);return;}
    if (s->board) {
        table_key(s, key);
        return;
    }
    if (s->search) {
        if (key == 27 || key == 3) {
            s->search = 0;
            line_set(s, s->saved);
        } else if (key == 13) {
            s->search = 0;
        } else if (key == 18)
            history_search(s);
        else {
            if (key == 8 || key == 127) {
                if (s->querylen)
                    s->querylen = prev(s->query, s->querylen);
                s->query[s->querylen] = 0;
            } else if (key >= 32 && key < 256 && s->querylen + 1 < sizeof s->query) {
                s->query[s->querylen++] = (char)key;
                s->query[s->querylen] = 0;
            }
            s->search_index = s->history_count;
            history_search(s);
        }
        prompt(s);
        return;
    }
    for (i = 0; i < s->binding_count; ++i)
        if (s->bindings[i].key == key) {
            if (s->bindings[i].fn)
                s->bindings[i].fn(s, key, s->bindings[i].context);
            else {
                ps_newline(s);
                ps_execute(s, s->bindings[i].command);
            }
            prompt(s);
            return;
        }
    if (key != 9)
        s->completion = 0;
    if (key == PS_KEY_F2) {
        table_open(s, 0);
        return;
    }
    if (key == PS_KEY_F3) {
        table_open(s, 2);
        return;
    }
    if (key == PS_KEY_F1) {
        ps_newline(s);
        ps_execute(s, "help");
    } else if (key == PS_KEY_F4) {
#ifndef PS_ASCII_ONLY
        s->io.flags ^= PS_COLOR;
#endif
    } else if (key == 13) {
        /* Discard the visible preview before submitting only the typed line. */
        if (*s->suggestion) ansi(s, "\033[K");
        ps_newline(s);
        s->line[s->len] = 0;
        if (s->overflow) {
            out(s, "error: line too long");
            ps_newline(s);
            s->status = 2;
        } else if (s->len) {
            ps_history_add(s, s->line);
            ps_execute(s, s->line);
        }
        s->len = s->pos = s->history_nav = s->overflow = 0;
        ps_secure_clear(s->line, sizeof s->line);
    } else if (key == 3) {
        if (*s->suggestion) ansi(s, "\033[K");
        ps_secure_clear(s->line, sizeof s->line);
        s->len = s->pos = 0;
        s->overflow = 0;
        if (s->job.active) {
            s->job.active = 0;
            s->job.cancelled = 1;
        }
        out(s, "^C");
        ps_newline(s);
    } else if (key == 4) {
        if (!s->len) {
            s->exit = 1;
            ps_close(s);
            return;
        }
        if (s->pos == s->len) ps_ring_bell(s);
        edit_delete(s->line, &s->len, &s->pos, 0);
    } else if (key == 7) {
        ps_ring_bell(s);
    } else if (key == 12) {
        s->prompt_rows_drawn=0;ansi(s, "\033[2J\033[H");
    } else if (key == 9) {
        completion(s);
    } else if (key == PS_KEY_LEFT || key == 2) {
        if (!s->pos) ps_ring_bell(s);
        s->pos = prev(s->line, s->pos);
    } else if (key == PS_KEY_RIGHT || key == 6) {
        if (s->pos == s->len) {
            const char *p = *s->suggestion ? s->suggestion : 0;
            if (p)
                line_set(s, p);
            else
                ps_ring_bell(s);
        } else
            s->pos = next(s->line, s->pos);
    } else if (key == PS_KEY_HOME || key == 1)
        s->pos = 0;
    else if (key == PS_KEY_END || key == 5) {
        const char *p = s->pos == s->len && *s->suggestion ? s->suggestion : 0;
        if (p)
            line_set(s, p);
        else
            s->pos = s->len;
    } else if (key == PS_KEY_WORDLEFT) {
        while (s->pos && s->line[s->pos - 1] == ' ')
            s->pos = prev(s->line, s->pos);
        while (s->pos && s->line[s->pos - 1] != ' ')
            s->pos = prev(s->line, s->pos);
    } else if (key == PS_KEY_WORDRIGHT) {
        while (s->line[s->pos] && s->line[s->pos] != ' ')
            s->pos = next(s->line, s->pos);
        while (s->line[s->pos] == ' ')
            ++s->pos;
    } else if (key == 8 || key == 127) {
        if (!s->pos) ps_ring_bell(s);
        edit_delete(s->line, &s->len, &s->pos, 1);
    } else if (key == PS_KEY_DELETE) {
        if (s->pos == s->len) ps_ring_bell(s);
        edit_delete(s->line, &s->len, &s->pos, 0);
    } else if (key == 21) {
        str(s->kill, s->line, PS_LINE);
        ps_secure_clear(s->line, sizeof s->line);
        s->len = s->pos = 0;
        s->overflow = 0;
    } else if (key == 11) {
        str(s->kill, s->line + s->pos, PS_LINE);
        s->line[s->pos] = 0;
        s->len = s->pos;
    } else if (key == 23) {
        unsigned p = s->pos;
        while (s->pos && s->line[s->pos - 1] == ' ')
            s->pos = prev(s->line, s->pos);
        while (s->pos && s->line[s->pos - 1] != ' ')
            s->pos = prev(s->line, s->pos);
        copy(s->kill, s->line + s->pos, p - s->pos);
        s->kill[p - s->pos] = 0;
        move(s->line + s->pos, s->line + p, s->len - p + 1);
        s->len -= p - s->pos;
    } else if (key == PS_KEY_KILLWORD && (s->io.flags & PS_EXTENDED_KEYS)) {
        unsigned end = s->pos;
        while (s->line[end] == ' ') ++end;
        while (s->line[end] && s->line[end] != ' ') end = next(s->line, end);
        copy(s->kill, s->line + s->pos, end - s->pos);
        s->kill[end - s->pos] = 0;
        move(s->line + s->pos, s->line + end, s->len - end + 1);
        s->len -= end - s->pos;
    } else if (key == 20 && (s->io.flags & PS_EXTENDED_KEYS)) {
        unsigned end = s->pos < s->len ? next(s->line, s->pos) : s->pos;
        unsigned mid = prev(s->line, end), begin = prev(s->line, mid), j;
        char first[4];
        if (mid > begin && mid - begin <= sizeof first) {
            for (j = 0; j < mid - begin; ++j) first[j] = s->line[begin + j];
            move(s->line + begin, s->line + mid, end - mid);
            copy(s->line + begin + end - mid, first, mid - begin);
            s->pos = end;
        } else ps_ring_bell(s);
    } else if (key == 25) {
        for (i = 0; s->kill[i]; ++i)
            edit_insert(s->line, &s->len, &s->pos, PS_LINE, (unsigned char)s->kill[i]);
    } else if (key == 18) {
        str(s->saved, s->line, PS_LINE);
        s->search = 1;
        s->query[0] = 0;
        s->querylen = 0;
        s->search_index = s->history_count;
        history_search(s);
    } else if (key == PS_KEY_UP || key == 16) {
        if (!s->history_nav)
            str(s->saved, s->line, PS_LINE);
        if (s->history_nav < s->history_count) {
            ++s->history_nav;
            line_set(s, ps_history_get(s, s->history_count - s->history_nav));
        } else ps_ring_bell(s);
    } else if (key == PS_KEY_DOWN || key == 14) {
        if (s->history_nav) {
            --s->history_nav;
            line_set(s, s->history_nav ? ps_history_get(s, s->history_count - s->history_nav)
                                       : s->saved);
        } else ps_ring_bell(s);
    } else if (key >= 32 && key < 256) {
        if (s->len + 1 >= PS_LINE) {
            ps_ring_bell(s);
            s->overflow = 1;
        } else
            edit_insert(s->line, &s->len, &s->pos, PS_LINE, key);
    }
    prompt(s);
}

PS_API void ps_key(ps_session *s,unsigned key){
#if PS_UNDO_DEPTH
    unsigned binding,bound=0;
    for(binding=0;binding<s->binding_count;++binding)if(s->bindings[binding].key==key){bound=1;break;}
    if(!s->board&&!s->document_open&&!s->menu_depth&&!s->file_count&&!s->form){
        if(!s->undo.frames)ps_undo_bind(&s->undo,&s->undo_frames[0][0],s->undo_positions,PS_LINE,PS_UNDO_DEPTH+1);
        if(!bound&&!s->search&&(key==31||key==PS_KEY_UNDO||key==PS_KEY_REDO)){
            ps_undo_record(&s->undo,s->line,s->pos);
            if(!ps_undo_step(&s->undo,key==PS_KEY_REDO,s->line,&s->pos))ps_ring_bell(s);
            s->len=length(s->line);s->overflow=0;s->completion=0;prompt(s);return;
        }
        if(!s->search)ps_undo_record(&s->undo,s->line,s->pos);
        shell_key(s,key);
        if(key==13||key==3)ps_undo_clear(&s->undo);
        else if(!s->search)ps_undo_record(&s->undo,s->line,s->pos);
        return;
    }
#endif
    shell_key(s,key);
}
static int fail(ps_session *s, const char *message) {
    ink(s, 3);
    out(s, "error: ");
    ps_write(s, message);
    reset(s);
    ps_newline(s);
    return 2;
}
static void hex(ps_session *s, ps_u32 n) {
    char b[9];
    unsigned i;
    for (i = 0; i < 8; ++i)
        b[i] = "0123456789abcdef"[(n >> (28 - i * 4)) & 15];
    b[8] = 0;
    out(s, b);
}
static void job_report(ps_session *s) {
    ps_write(s, s->job.name);
    out(s, "  ");
    ps_number(s, s->job.done);
    out(s, "/");
    ps_number(s, s->job.total);
    out(s, s->job.cancelled ? " cancelled" : s->job.done < s->job.total ? " running" : " complete");
    out(s, "  fnv1a=");
    hex(s, s->job.hash);
    ps_newline(s);
}
PS_API void ps_ring_bell(ps_session *s) {
    if (s && (s->io.flags & PS_INTERACTIVE) && !(s->io.flags & PS_NO_BELL))
        out(s, "\a");
}
static const char *command_category(const ps_command *c) {
    return c->category && *c->category ? c->category : "Commands";
}
#include "portable_textflow.inc"
#include "portable_help.inc"
#include "portable_report.inc"
static void segment_sink(void *context,const unsigned char *p,size_t n){
    raw((ps_session *)context,(const char *)p,(unsigned)n);
}
static int builtin(ps_session *s, int argc, const char *const *a, void *ctx) {
    unsigned i, j;
    const char *cmd = a[0];
    (void)ctx;
    if(equal(cmd,"edit")){
        if(argc>2||(argc==2&&!equal(a[1],"--framed")))return fail(s,"edit [--framed]");
        if(argc==2)ps_editor_set_style(s,PS_EDITOR_HEADER|PS_EDITOR_BORDER);
        return ps_editor_open(s,"Session notes (memory)",s->scratch,document_save_scratch,s)?0:fail(s,"editor unavailable (interactive terminal required)");
    }
    if(equal(cmd,"files")){
        if(argc!=1)return fail(s,"files");
        return ps_file_picker_open(s,"Session files (memory)",session_files,2,session_file_pick,0)?0:fail(s,"file picker requires interactive terminal");
    }
    if(equal(cmd,"menu")){
        int result=0;unsigned first=1,style=PS_MENU_TABLE|PS_MENU_NUMBERS;
        for(;first<(unsigned)argc;++first){if(equal(a[first],"--list")&&PS_MENU_LIST_VIEW)style=PS_MENU_LIST;else if(equal(a[first],"--plain"))style=0;else if(equal(a[first],"--no-numbers"))style&=~PS_MENU_NUMBERS;else if(equal(a[first],"--details"))style|=PS_MENU_DETAILS;else break;}
        ps_menu_set_style(s,style);ps_menu_open(s,&default_menu);
        for(i=first;i<(unsigned)argc;++i){
            int n;const ps_menu *m=s->menu_depth?s->menu_stack[s->menu_depth-1]:0;
            if(!m||!integer(a[i],&n)||n<0||(unsigned)n>m->count||
               (i+1<(unsigned)argc&&n&&!m->items[n-1].submenu)){
                ps_menu_close(s);return fail(s,"menu [NUMBER ...]");
            }
            result=ps_menu_choose(s,n);
            if(result){ps_menu_close(s);return result;}
        }
        if(!(s->io.flags&PS_INTERACTIVE))ps_menu_close(s);
        return result;
    }
    if (equal(cmd, "help")) {
        const char *name = 0;
        unsigned layout = PS_HELP_HEADINGS;
        for (i = 1; i < (unsigned)argc; ++i) {
            if (equal(a[i], "--compact")) layout = PS_HELP_COMPACT;
            else if (equal(a[i], "--headings")) layout = PS_HELP_HEADINGS;
            else if (!name) name = a[i];
            else return fail(s, "help [--headings|--compact] [command]");
        }
        return ps_print_help(s, name, layout) ? 0 : fail(s, "Unknown command");
    }
    if (equal(cmd, "bell")) {
        ps_ring_bell(s);
        return 0;
    }
    if (equal(cmd, "echo")) {
        for (i = 1; i < (unsigned)argc; ++i) {
            if (i > 1)
                out(s, " ");
            ps_write(s, a[i]);
        }
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "args")) {
        for (i = 1; i < (unsigned)argc; ++i) {
            ps_number(s, i - 1);
            out(s, " [");
            ps_number(s, length(a[i]));
            out(s, "] ");
            ps_write(s, a[i]);
            ps_newline(s);
        }
        return 0;
    }
    if (equal(cmd, "controls")) {
        int result;
        s->notice[0] = 0;
        result = controls_command(s, argc, a);
        if (result && !s->board)
            return fail(s, s->notice[0] ? s->notice
                                        : "controls [open [page]|dashboard|list|get ID|set ID VALUE]");
        return result;
    }
    if (equal(cmd, "board")) {
        unsigned tab = 0;
        if (argc > 2)
            return fail(s, "board [tasks|variables|jobs|session]");
        if (argc == 2) {
            if (equal(a[1], "variables"))
                tab = 1;
            else if (equal(a[1], "jobs"))
                tab = 2;
            else if (equal(a[1], "session"))
                tab = 3;
            else if (!equal(a[1], "tasks"))
                return fail(s, "unknown tab");
        }
        if (!(s->io.flags & PS_INTERACTIVE))
            return fail(s, "board requires interactive mode");
        s->control_view = s->dialog = 0;
        table_open(s, tab);
        return 0;
    }
    if (equal(cmd, "task")) {
        if (argc == 1 || (argc == 2 && equal(a[1], "list"))) {
            for (i = 0; i < s->task_count; ++i) {
                ps_number(s, s->tasks[i].id);
                out(s, s->tasks[i].done ? " [x] " : " [ ] ");
                ps_write(s, s->tasks[i].title);
                ps_newline(s);
            }
            return 0;
        }
        if (argc == 3 && equal(a[1], "add")) {
            if (!add_task(s, a[2]))
                return fail(s, "task must be 1..127 bytes; table capacity 32");
            ps_number(s, s->tasks[s->task_count - 1].id);
            ps_newline(s);
            return 0;
        }
        if (argc >= 3) {
            int id;
            if (!integer(a[2], &id) || id < 1)
                return fail(s, "invalid task id");
            for (i = 0; i < s->task_count && s->tasks[i].id != (unsigned)id; ++i) {
            }
            if (i == s->task_count)
                return fail(s, "task not found");
            if (argc == 3 && equal(a[1], "done")) {
                s->tasks[i].done ^= 1;
                return 0;
            }
            if (argc == 3 && equal(a[1], "remove")) {
                move(s->tasks + i, s->tasks + i + 1, (s->task_count - i - 1) * sizeof(Task));
                --s->task_count;
                return 0;
            }
            if (argc == 4 && equal(a[1], "rename")) {
                if (!*a[3] || length(a[3]) >= PS_VALUE)
                    return fail(s, "invalid title length");
                str(s->tasks[i].title, a[3], PS_VALUE);
                return 0;
            }
        }
        return fail(s, "task add TITLE | done ID | remove ID | rename ID TITLE | list");
    }
    if (equal(cmd, "set") || equal(cmd, "get") || equal(cmd, "unset")) {
        int set = equal(cmd, "set"), del = equal(cmd, "unset");
        if ((set && argc != 3) || (del && argc != 2) || (!set && !del && argc > 2))
            return fail(s, "set KEY VALUE | get [KEY] | unset KEY");
        if (argc == 1) {
            for (i = 0; i < s->var_count; ++i) {
                ps_write(s, s->vars[i].key);
                out(s, "=");
                ps_write(s, s->vars[i].value);
                ps_newline(s);
            }
            return 0;
        }
        for (i = 0; i < s->var_count && !equal(s->vars[i].key, a[1]); ++i) {
        }
        if (set) {
            if (!*a[1] || length(a[1]) >= 32 || length(a[2]) >= PS_VALUE)
                return fail(s, "key/value too long or empty key");
            for (j = 0; a[1][j]; ++j)
                if (!((a[1][j] >= 'a' && a[1][j] <= 'z') || (a[1][j] >= 'A' && a[1][j] <= 'Z') ||
                      (a[1][j] >= '0' && a[1][j] <= '9') || a[1][j] == '_' || a[1][j] == '-' ||
                      a[1][j] == '.'))
                    return fail(s, "key requires letters, digits, _, - or .");
            if (i == PS_ROWS)
                return fail(s, "variable capacity reached");
            if (i == s->var_count)
                ++s->var_count;
            str(s->vars[i].key, a[1], 32);
            str(s->vars[i].value, a[2], PS_VALUE);
            return 0;
        }
        if (i == s->var_count)
            return fail(s, "key not found");
        if (del) {
            move(s->vars + i, s->vars + i + 1, (s->var_count - i - 1) * sizeof(Variable));
            --s->var_count;
        } else {
            ps_write(s, s->vars[i].value);
            ps_newline(s);
        }
        return 0;
    }
    if (equal(cmd, "text")) {
        char b[PS_LINE];
        unsigned n = 0, start = 0, end, sep = 0;
        if (argc != 3)
            return fail(s, "text upper|lower|trim|slug TEXT");
        end = length(a[2]);
        if (equal(a[1], "trim")) {
            while (a[2][start] == ' ' || a[2][start] == '\t' || a[2][start] == '\n')
                ++start;
            while (end > start &&
                   (a[2][end - 1] == ' ' || a[2][end - 1] == '\t' || a[2][end - 1] == '\n'))
                --end;
        } else if (!equal(a[1], "upper") && !equal(a[1], "lower") && !equal(a[1], "slug"))
            return fail(s, "unknown transform");
        for (i = start; i < end && n + 1 < PS_LINE; ++i) {
            unsigned char c = (unsigned char)a[2][i];
            if (equal(a[1], "upper") && c >= 'a' && c <= 'z')
                c -= 32;
            else if ((equal(a[1], "lower") || equal(a[1], "slug")) && c >= 'A' && c <= 'Z')
                c += 32;
            if (equal(a[1], "slug")) {
                if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
                    sep = n ? 1 : 0;
                    continue;
                }
                if (sep && n + 2 < PS_LINE)
                    b[n++] = '-';
                sep = 0;
            }
            b[n++] = (char)c;
        }
        b[n] = 0;
        ps_write(s, b);
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "calc")) {
        int x, y, r = 0;
        unsigned ax, ay, lim;
        char op;
        if (argc != 4 || !integer(a[1], &x) || !integer(a[3], &y) || length(a[2]) != 1)
            return fail(s, "calc INTEGER +|-|*|/|% INTEGER");
        op = *a[2];
        if (op == '+') {
            if ((y > 0 && x > 2147483647 - y) || (y < 0 && x < (-2147483647 - 1) - y))
                return fail(s, "integer overflow");
            r = x + y;
        } else if (op == '-') {
            if ((y < 0 && x > 2147483647 + y) || (y > 0 && x < (-2147483647 - 1) + y))
                return fail(s, "integer overflow");
            r = x - y;
        } else if (op == '*') {
            ax = x < 0 ? 0u - (unsigned)x : (unsigned)x;
            ay = y < 0 ? 0u - (unsigned)y : (unsigned)y;
            lim = ((x < 0) != (y < 0)) ? 2147483648u : 2147483647u;
            if (ay && ax > lim / ay)
                return fail(s, "integer overflow");
            ax *= ay;
            r = ((x < 0) != (y < 0)) ? (ax == 2147483648u ? (-2147483647 - 1) : -(int)ax) : (int)ax;
        } else if (op == '/' || op == '%') {
            if (!y)
                return fail(s, "division by zero");
            if (x == (-2147483647 - 1) && y == -1)
                return fail(s, "integer overflow");
            r = op == '/' ? x / y : x % y;
        } else
            return fail(s, "unknown operator");
        signed_number(s, r);
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "hash")) {
        const char *rounds;
        int n = 1;
        if (!(argc == 2 || (argc == 3 && prefix(a[2], "--rounds=")) ||
              (argc == 4 && equal(a[2], "--rounds"))))
            return fail(s, "hash TEXT [--rounds N]");
        rounds = ps_option(argc, a, "rounds");
        if (rounds && (!integer(rounds, &n) || n < 1 || n > 1000000))
            return fail(s, "rounds must be 1..1000000");
        if (s->job.active)
            return fail(s, "job already running");
        str(s->job_data, a[1], PS_LINE);
        s->job_length = length(s->job_data);
        str(s->job.name, "FNV-1a stream", 40);
        s->job.total = (unsigned)n;
        s->job.done = 0;
        s->job.active = 1;
        s->job.cancelled = 0;
        s->job.hash = 2166136261u;
        notice(s, "Hash job started");
        return 0;
    }
    if (equal(cmd, "jobs")) {
        if (argc == 2 && equal(a[1], "cancel")) {
            s->job.active = 0;
            s->job.cancelled = 1;
            notice(s, "Job cancelled");
        } else if (argc != 1)
            return fail(s, "jobs [cancel]");
        if (s->job.total)
            job_report(s);
        return 0;
    }
    if (equal(cmd, "history")) {
        if (argc == 2 && equal(a[1], "clear")) {
            ps_history_clear(s);
            return 0;
        }
        if (argc != 1)
            return fail(s, "history [clear]");
        for (i = 0; i < s->history_count; ++i) {
            ps_number(s, i + 1);
            out(s, "  ");
            ps_write(s, ps_history_get(s, i));
            ps_newline(s);
        }
        return 0;
    }
    if (equal(cmd, "bind")) {
        unsigned key;
        if (argc == 1) {
            for (i = 0; i < s->binding_count; ++i) {
                ps_number(s, s->bindings[i].key);
                out(s, "  ");
                ps_write(s, s->bindings[i].command);
                ps_newline(s);
            }
            return 0;
        }
        if (argc != 3 || a[1][0] != 'F' || a[1][1] < '1' || a[1][1] > '4' || a[1][2] ||
            length(a[2]) >= 64)
            return fail(s, "bind F1|F2|F3|F4 COMMAND");
        key = PS_KEY_F1 + (unsigned)(a[1][1] - '1');
        if (ps_bind(s, key, 0, 0))
            return fail(s, "binding capacity reached");
        for (i = 0; i < s->binding_count; ++i)
            if (s->bindings[i].key == key)
                str(s->bindings[i].command, a[2], 64);
        return 0;
    }
    if(equal(cmd,"segments")){
        unsigned short digits[PS_SEG_MAX_CELLS];int count;
        ps_segment_style display=ps_segment_default_style();
        if(argc<2)return fail(s,"segments TEXT [--large] [--red] [--mono] [--ascii]");
        display.unicode=!!(s->io.flags&PS_UNICODE);
        display.color=(s->io.flags&(PS_INTERACTIVE|PS_COLOR))==(PS_INTERACTIVE|PS_COLOR);
        for(i=2;i<(unsigned)argc;++i){
            if(equal(a[i],"--large")){display.horizontal=4;display.vertical=3;}
            else if(equal(a[i],"--red"))display.foreground=203;
            else if(equal(a[i],"--mono"))display.color=0;
            else if(equal(a[i],"--ascii"))display.unicode=0;
            else return fail(s,"unknown display option");
        }
        count=ps_segment_parse(a[1],digits,PS_SEG_MAX_CELLS);
        if(count<=0)return fail(s,"unsupported display text or too many cells");
        if(!ps_segment_width(digits,(unsigned)count,&display)||ps_segment_width(digits,(unsigned)count,&display)>=s->io.columns)
            return fail(s,"display does not fit terminal");
        frame_begin(s);
        ps_segment_render(digits,(unsigned)count,&display,s->io.columns-1,segment_sink,s);
        frame_end(s);s->prompt_rows_drawn=0;return 0;
    }
    if(equal(cmd,"prompt")){
        static const char *const names[]={"default","heading","success","warning","error","green","muted","command","blue"};
        ps_prompt_part parts[16];unsigned n=0,j;
        if(argc==2&&equal(a[1],"reset"))return ps_set_prompt_parts(s,0,0);
        if(argc<3 || !(argc&1))return fail(s,"prompt reset | STYLE TEXT [STYLE TEXT ...]");
        for(i=1;i<(unsigned)argc;i+=2){
            for(j=0;j<9&&!equal(a[i],names[j]);++j){}
            if(j==9||n==16)return fail(s,"unknown prompt style");
            parts[n].text=a[i+1];parts[n++].style=j;
        }
        return ps_set_prompt_parts(s,parts,n)?fail(s,"prompt exceeds limits or contains controls"):0;
    }
    if (equal(cmd, "theme") || equal(cmd, "display")) {
        const char *choice;
        if(equal(cmd,"display")){
            if(argc!=3)return fail(s,"display color rgb|ansi|mono; display glyphs unicode|ascii");
            if(equal(a[1],"color")&&(equal(a[2],"rgb")||equal(a[2],"ansi")||equal(a[2],"mono")))
                choice=equal(a[2],"rgb")?"color":a[2];
            else if(equal(a[1],"glyphs")&&(equal(a[2],"unicode")||equal(a[2],"ascii")))choice=a[2];
            else return fail(s,"unknown display setting");
        }else{
            if(argc!=2)return fail(s,"theme default|cyan|violet|red|onedark");
            choice=equal(a[1],"semantic")?"default":equal(a[1],"lumen")?"cyan":a[1];
        }
#ifdef PS_ASCII_ONLY
        if (equal(choice, "color") || equal(choice, "unicode") || equal(choice, "violet") ||
            equal(choice, "red") || equal(choice, "cyan") || equal(choice, "onedark") || equal(choice, "default") ||
            equal(choice, "ansi"))
            return fail(s, "disabled in ASCII-only build");
#endif
        if (equal(choice, "violet") || equal(choice, "red") || equal(choice, "cyan") ||
            equal(choice, "onedark") || equal(choice, "default")) {
            s->theme = equal(choice, "default") ? PS_THEME_DEFAULT : equal(choice, "onedark") ? 3
                       : equal(choice, "cyan") ? 2
                       : equal(choice, "red")   ? 1
                                              : 0;
        } else if (equal(choice, "color"))
            s->io.flags |= PS_COLOR | PS_TRUECOLOR;
        else if (equal(choice, "ansi")) {
            s->io.flags |= PS_COLOR;
            s->io.flags &= ~PS_TRUECOLOR;
        } else if (equal(choice, "mono")) {
            s->io.flags &= ~PS_COLOR;
            reset(s);
        } else if (equal(choice, "ascii"))
            s->io.flags &= ~PS_UNICODE;
        else if (equal(choice, "unicode"))
            s->io.flags |= PS_UNICODE;
        else
            return fail(s, "unknown theme");
        if (s->board) {
            reset(s);
            s->canvas_kind=0;
        }
        s->dirty = 1;
        return 0;
    }
    if (equal(cmd, "clear")) {
        s->prompt_rows_drawn=0;ansi(s, "\033[2J\033[H");
        return 0;
    }
    if (equal(cmd, "status")) {
#if PS_REPORT
        if(argc==2&&equal(a[1],"--report")){
            char storage[11],executions[11],tasks[11],received[11],sent[11];
            ps_report_row rows[]={
                {"Session",0,PS_VERSION,PS_REPORT_SECTION},
                {"Storage","caller-owned bytes",storage,PS_REPORT_NUMBER},
                {"Heap","core allocations","0",PS_REPORT_NUMBER},
                {"Backend",0,ps_memory_backend(),0},
                {"Activity",0,0,PS_REPORT_SECTION|PS_REPORT_GAP},
                {"Commands","executed",executions,PS_REPORT_NUMBER},
                {"Tasks",0,tasks,PS_REPORT_NUMBER},
                {"Input","bytes",received,PS_REPORT_NUMBER},
                {"Output","bytes before this report",sent,PS_REPORT_NUMBER}
            };
            dec(storage,sizeof *s);dec(executions,s->executions);dec(tasks,s->task_count);
            dec(received,s->bytes_in);dec(sent,s->bytes_out);
            ps_print_report(s,rows,sizeof rows/sizeof rows[0],PS_REPORT_DOTS);
            return 0;
        }
#endif
        if(argc!=1)return fail(s,"status [--report]");
        out(s, "version=" PS_VERSION " session_bytes=");
        ps_number(s, sizeof *s);
        out(s, " heap_bytes=0 memory_backend=");
        out(s, ps_memory_backend());
        out(s, " commands=");
        ps_number(s, s->executions);
        out(s, " tasks=");
        ps_number(s, s->task_count);
        out(s, " input_bytes=");
        ps_number(s, s->bytes_in);
        out(s, " output_bytes=");
        ps_number(s, s->bytes_out);
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "exit")) {
        s->exit = 1;
        ps_close(s);
        return 0;
    }
    return fail(s, "unknown command");
}
PS_API void ps_set_executor(ps_session *s, ps_command_fn run, void *context) {
    if (s) { s->executor = run; s->executor_context = context; }
}
PS_API void ps_set_status(ps_session *s,int status) { if(s)s->status=status; }
PS_API void ps_set_line_executor(ps_session *s,ps_line_fn run,void *context) {
    if(s){s->line_executor=run;s->line_context=context;}
}
PS_API unsigned ps_output_flags(ps_session *s,unsigned flags) {
    unsigned mask=PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR,old=s->io.flags&mask;
    s->io.flags=(s->io.flags&~mask)|(flags&mask);return old;
}
PS_API void ps_exchange_output(ps_session *s,ps_write_fn *write,void **context) {
    ps_write_fn old;void *ctx;
    if(!s||!write||!context||!*write)return;
    old=s->io.write;ctx=s->io.context;s->io.write=*write;s->io.context=*context;*write=old;*context=ctx;
}
PS_API int ps_execute_argv(ps_session *s, int argc, const char *const *a) {
    unsigned i;
    if (argc <= 0)
        return 0;
    if (argc > PS_ARGS || !a || !a[0])
        return fail(s, "invalid argv");
    ++s->executions;
    for (i = 0; i < s->command_count; ++i)
        if (name_match(s, a[0], s->commands[i].name, 0)) {
            const char *canonical[PS_ARGS];
            unsigned k;
            for (k = 0; k < (unsigned)argc; ++k)
                canonical[k] = a[k];
            canonical[0] = s->commands[i].name;
            s->executing = 1;
            s->status = s->commands[i].run(s, argc, canonical, s->commands[i].context);
            s->executing = 0;
            return s->status;
        }
    if (s->executor) {
        s->executing = 1;
        s->status = s->executor(s, argc, a, s->executor_context);
        s->executing = 0;
        return s->status;
    }
    s->status = fail(s, "unknown command; use help or Tab");
    return s->status;
}
PS_API int ps_execute(ps_session *s, const char *line) {
    ps_args a;
    char name[PS_LINE];
    unsigned n = 0, i;
    if (!line || length(line) >= PS_LINE)
        return s->status = fail(s, "line too long");
    while (line[n] && line[n] != ' ' && line[n] != '\t') {
        name[n] = line[n];
        ++n;
    }
    name[n] = 0;
    for (i = 0; i < s->command_count; ++i)
        if (s->raw_commands[i] && name_match(s, name, s->commands[i].name, 0)) {
            ++s->executions;
            s->executing = 1;
            s->status = s->raw_commands[i](s, line + n + (line[n] != 0), s->commands[i].context);
            s->executing = 0;
            return s->status;
        }
    if(s->line_executor)return s->status=s->line_executor(s,line,s->line_context);
    if (ps_parse(line, &a)) {
        out(s, "parse@");
        ps_number(s, a.error_at);
        out(s, " ");
        s->status = fail(s, a.error);
        return s->status;
    }
    return ps_execute_argv(s, a.argc, (const char *const *)a.argv);
}
PS_API void ps_progress(ps_session *s, const char *name, unsigned done, unsigned total) {
    str(s->job.name, name, 40);
    s->job.total = min(total, 1000000);
    s->job.done = min(done, s->job.total);
    s->job.active = 0;
    s->job.cancelled = 0;
    s->dirty = 1;
}
#include "portable_mouse.inc"
static void escape_key(ps_session *s) {
    const char *p = s->escbuf;
    unsigned key = 0;
    if(p[0]=='['&&p[1]=='<'){
        ps_mouse_event event;
        if(PS_MOUSE&&ps_mouse_parse(p+1,&event)){
            if(s->file_count||s->menu_depth||ps_form_active(s))workspace_mouse(s,&event);else table_mouse(s,&event);
        }
        return;
    }
    if (equal(p, "[A") || equal(p, "OA"))
        key = PS_KEY_UP;
    else if (equal(p, "[B") || equal(p, "OB"))
        key = PS_KEY_DOWN;
    else if (equal(p, "[C") || equal(p, "OC"))
        key = PS_KEY_RIGHT;
    else if (equal(p, "[D") || equal(p, "OD"))
        key = PS_KEY_LEFT;
    else if (equal(p, "[H") || equal(p, "OH") || equal(p, "[1~") || equal(p, "[7~"))
        key = PS_KEY_HOME;
    else if (equal(p, "[F") || equal(p, "OF") || equal(p, "[4~") || equal(p, "[8~"))
        key = PS_KEY_END;
    else if (equal(p, "[3~"))
        key = PS_KEY_DELETE;
    else if (equal(p, "[5~"))
        key = PS_KEY_PAGEUP;
    else if (equal(p, "[6~"))
        key = PS_KEY_PAGEDOWN;
    else if (equal(p, "[Z"))
        key = PS_KEY_BACKTAB;
    else if (equal(p, "OP") || equal(p, "[11~"))
        key = PS_KEY_F1;
    else if (equal(p, "OQ") || equal(p, "[12~"))
        key = PS_KEY_F2;
    else if (equal(p, "OR") || equal(p, "[13~"))
        key = PS_KEY_F3;
    else if (equal(p, "OS") || equal(p, "[14~"))
        key = PS_KEY_F4;
    else if (equal(p, "[1;5D"))
        key = PS_KEY_WORDLEFT;
    else if (equal(p, "[1;5C"))
        key = PS_KEY_WORDRIGHT;
    else if (equal(p, "[200~")) {
        if(s->document_open&&!s->document_dialog)document_snapshot(s);
#if PS_UNDO_DEPTH
        if(!s->board&&!s->form&&!s->document_open&&!s->menu_depth&&!s->file_count){
            if(!s->undo.frames)ps_undo_bind(&s->undo,&s->undo_frames[0][0],s->undo_positions,PS_LINE,PS_UNDO_DEPTH+1);
            ps_undo_record(&s->undo,s->line,s->pos);
        }
#endif
        s->paste = 1;
        s->paste_match = 0;
    }
    if (key)
        ps_key(s, key);
}
static void input_byte(ps_session *s, unsigned c) {
    if (s->paste) {
        static const char end[] = "\033[201~";
        if (c == (unsigned char)end[s->paste_match]) {
            if (++s->paste_match == 6) {
                s->paste = 0;
                s->paste_match = 0;
#if PS_UNDO_DEPTH
                if(s->undo.frames&&!s->board&&!s->form&&!s->document_open&&!s->menu_depth&&!s->file_count)ps_undo_record(&s->undo,s->line,s->pos);
#endif
                prompt(s);
            }
            return;
        }
        if (s->paste_match)
            s->paste_match = 0;
#if PS_FORM
        if(s->form){if(c>=32&&c!=127)form_key(s,c);return;}
#endif
        if(s->document_open){document_paste(s,c);return;}
        if(s->menu_depth||s->file_count)return; /* Pasted newlines must never activate menu actions. */
        if (c == '\r' || c == '\n' || c == '\t')
            c = ' ';
        if (c >= 32 && c != 127) {
            if (s->board && s->editing && (!s->control_view || !s->dialog_focus)) {
                edit_insert(s->edit, &s->editlen, &s->editpos, s->control_view ? 64 : PS_VALUE, c);
                s->dirty = 1;
            } else if (!s->board) {
                if (s->len + 1 >= PS_LINE)
                    s->overflow = 1;
                else
                    edit_insert(s->line, &s->len, &s->pos, PS_LINE, c);
            }
        }
        return;
    }
    if (s->esc) {
        if (s->esc == 3) {
            if (c >= 64 && c <= 126)
                s->esc = 0;
            return;
        }
        if (s->esc == 1) {
            if (c == '[' || c == 'O') {
                s->escbuf[0] = (char)c;
                s->esc_len = 1;
                s->esc = 2;
                return;
            }
            s->esc = 0;
            if ((s->io.flags & PS_LEGACY_KEYS) &&
                (c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'H' || c == 'F')) {
                s->escbuf[0] = 'O';
                s->escbuf[1] = (char)c;
                s->escbuf[2] = 0;
                escape_key(s);
                return;
            }
            if ((s->io.flags & PS_EXTENDED_KEYS) && (c == 'd' || c == 127 || c == 8)) {
                ps_key(s, c == 'd' ? PS_KEY_KILLWORD : 23);
                return;
            }
            if(c=='_'){ps_key(s,PS_KEY_REDO);return;}
            if (c == 'b') {
                ps_key(s, PS_KEY_WORDLEFT);
                return;
            }
            if (c == 'f') {
                ps_key(s, PS_KEY_WORDRIGHT);
                return;
            }
            ps_key(s, 27);
        } else {
            if (s->esc_len + 1 < sizeof s->escbuf)
                s->escbuf[s->esc_len++] = (char)c;
            else {
                s->esc = 3;
                return;
            }
            s->escbuf[s->esc_len] = 0;
            if (c >= 64 && c <= 126) {
                s->esc = 0;
                escape_key(s);
            }
            return;
        }
    }
    if (c == 27) {
        s->esc = 1;
        s->esc_len = 0;
        s->esc_time = s->now;
        return;
    }
    if (c == '\n' && s->suppress_lf) {
        s->suppress_lf = 0;
        return;
    }
    if (c == 0 && s->suppress_lf) {
        s->suppress_lf = 0;
        return;
    }
    s->suppress_lf = c == '\r';
    if (c == '\n')
        c = 13;
    if(s->menu_depth||s->document_open||s->file_count){ps_key(s,c);return;}
    if (!(s->io.flags & PS_INTERACTIVE)) {
        if (c == 13) {
            if (s->overflow) {
                s->status = fail(s, "line too long");
            } else if (s->len)
                ps_execute(s, s->line);
            ps_secure_clear(s->line, sizeof s->line);
            s->len = s->pos = s->overflow = 0;
        } else if (c >= 32 || c == 9) {
            if (s->len + 1 < PS_LINE) {
                s->line[s->len++] = (char)c;
                s->line[s->len] = 0;
                s->pos = s->len;
            } else
                s->overflow = 1;
        }
        return;
    }
    ps_key(s, c);
}
static void telnet_byte(ps_session *s, unsigned c) {
    unsigned char decoded = 0;
    int event = ps_telnet_feed(&s->telnet, (unsigned char)c, telnet_write, s, &decoded);
    if (event == PS_TN_DATA)
        input_byte(s, decoded);
    else if (event == PS_TN_RESIZE)
        ps_resize(s, s->telnet.columns, s->telnet.rows);
    else if (event == PS_TN_TERMINAL) {
        if (!ps_telnet_ansi(&s->telnet)) {
            if (s->board)
                table_close(s);
            s->io.flags &= ~(PS_INTERACTIVE | PS_COLOR | PS_UNICODE);
        }
        s->dirty = 1;
    }
}
PS_API void ps_feed(ps_session *s, const void *data, unsigned n) {
    const ps_byte *p = data;
    s->bytes_in += n;
    while (n-- && !s->exit) {
        if (s->io.flags & PS_TELNET)
            telnet_byte(s, *p++);
        else
            input_byte(s, *p++);
    }
    if (s->board && s->dirty)
        render(s);
}
PS_API void ps_tick(ps_session *s, ps_u32 now) {
    unsigned rounds = 0, i;
    s->now = now;
    if (s->esc && (ps_u32)(now - s->esc_time) >= 60) {
        unsigned is_escape = s->esc == 1;
        s->esc = 0;
        if (is_escape)
            ps_key(s, 27);
    }
    if (s->job.active) {
        while (s->job.done < s->job.total && rounds++ < 128) {
            for (i = 0; i < s->job_length; ++i) {
                s->job.hash ^= (ps_byte)s->job_data[i];
                s->job.hash *= 16777619u;
            }
            ++s->job.done;
        }
        s->dirty = 1;
        if (s->job.done == s->job.total) {
            s->job.active = 0;
            notice(s, "Hash complete");
            if (!s->board) {
                if (s->io.flags & PS_INTERACTIVE)
                    ansi(s, "\r\033[2K");
                job_report(s);
                prompt(s);
            }
        }
    }
    if (s->board && (s->dirty || (s->tab == 3 && now / 1000 != s->last_paint / 1000)) &&
        (ps_u32)(now - s->last_paint) >= 100)
        render(s);
}
PS_API void ps_resize(ps_session *s, unsigned columns, unsigned rows) {
    s->io.columns = columns < 8 ? 8 : min(columns, 240);
    s->io.rows = rows < 4 ? 4 : min(rows, 80);
    s->dirty = 1;
#if PS_FORM
    if(s->form){form_render(s);return;}
#endif
    if(s->document_open){document_render(s);return;}
    if(s->file_count){file_render(s);return;}
    if(s->menu_depth){ps_menu_refresh(s);return;}
    if (s->board) {
        render(s);
    } else
        prompt(s);
}
PS_API void ps_finish(ps_session *s) {
    if (s->len || s->overflow) {
        if (s->overflow)
            s->status = fail(s, "line too long");
        else
            ps_execute(s, s->line);
        s->len = s->pos = s->overflow = 0;
        ps_secure_clear(s->line, sizeof s->line);
    }
    while (s->job.active)
        ps_tick(s, s->now + 1);
}
PS_API void ps_close(ps_session *s) {
    ps_form_close(s);
    ps_file_picker_close(s);
    if(s->document_open)document_close(s);
    ps_menu_close(s);
    if(PS_MOUSE && s->board)ansi(s,"\033[?1002l\033[?1006l");
    if (s->board) {
        ansi(s, "\033[?1049l");
        s->board = 0;
    }
    ansi(s, "\033[0m\033[?25h\033[?2004l");
    s->started = 0;
}
PS_API int ps_exited(const ps_session *s) {
    return (int)s->exit;
}
PS_API int ps_status(const ps_session *s) {
    return s->status;
}
