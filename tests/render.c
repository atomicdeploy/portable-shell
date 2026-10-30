#ifdef PS_RICH_REPLAY
#include "portable_rich.h"
static unsigned attributes;
#endif
/* Small, purpose-built ANSI replay renderer for verification. No dependencies.
   This is a test utility, not the shell's terminal renderer. GDB exports SVG. */
#ifndef PS_REPLAY_ONLY
#include "portable_shell.h"
#include "portable_shell_device.h"
#endif
#ifndef W
#define W 96
#endif
#ifndef H
#define H 24
#endif
typedef struct {
    char text[5];
    unsigned fg, bg, span;
#ifdef PS_RICH_REPLAY
    unsigned attributes;
#endif
} Cell;
static Cell screen[H][W];
static unsigned row, col, fg = 0xf1e7ff, bg = 0x17131f, escape, used;
static unsigned saved_row, saved_col;
static unsigned default_fg = 0xf1e7ff, default_bg = 0x17131f;
static char sequence[64], utf[5];
static unsigned utf_n, utf_left;
#ifndef PS_REPLAY_ONLY
static union {
    void *align;
    unsigned char bytes[40000];
} memory;
char preview_tasks[400000], preview_variables[400000], preview_jobs[400000],
    preview_session[400000], preview_edit[400000], preview_ascii[400000], preview_red[400000],
    preview_lumen[400000];
char preview_device_commands[400000];
char preview_value_edit[400000];
unsigned preview_value_edit_n;
char preview_onedark[400000];
unsigned preview_onedark_n;
char preview_controls[400000], preview_control_settings[400000], preview_control_inline[400000],
    preview_control_dialog[400000], preview_control_confirm[400000], preview_control_ascii[400000];
unsigned preview_controls_n, preview_control_settings_n, preview_control_inline_n,
    preview_control_dialog_n, preview_control_confirm_n, preview_control_ascii_n;
char preview_control_columns[400000],preview_control_collapsed[400000];
unsigned preview_control_columns_n,preview_control_collapsed_n;
char preview_control_compact[400000];unsigned preview_control_compact_n;
char preview_control_inside[400000];unsigned preview_control_inside_n;
unsigned preview_device_commands_n;
unsigned preview_tasks_n, preview_variables_n, preview_jobs_n, preview_session_n, preview_edit_n,
    preview_ascii_n, preview_red_n, preview_lumen_n;
#endif
#ifndef PS_REPLAY_ONLY
static unsigned len(const char *p) {
    unsigned n = 0;
    while (p[n])
        ++n;
    return n;
}
#endif
static void blank(unsigned r, unsigned c) {
    screen[r][c].text[0] = ' ';
    screen[r][c].text[1] = 0;
    screen[r][c].span = 1;
    screen[r][c].fg = fg;
    screen[r][c].bg = bg;
}
static unsigned palette(unsigned n) {
    static const unsigned colors[16] = {0x000000, 0xcc4444, 0x55aa55, 0xccaa55, 0x6688cc, 0xaa66cc,
                                        0x55bbbb, 0xdddddd, 0x666666, 0xff6666, 0x88dd88, 0xffdd88,
                                        0x88aaff, 0xdd99ff, 0x88eeee, 0xffffff};
    if (n < 16)
        return colors[n];
    if (n >= 232) {
        unsigned v = 8 + 10 * (n - 232);
        return (v << 16) | (v << 8) | v;
    }
    n -= 16;
    {
        unsigned r = n / 36, g = (n / 6) % 6, b = n % 6;
        r = r ? 55 + r * 40 : 0;
        g = g ? 55 + g * 40 : 0;
        b = b ? 55 + b * 40 : 0;
        return (r << 16) | (g << 8) | b;
    }
}
static void csi(char end) {
    unsigned a[16] = {0}, n = 0, i = 0, r, c;
    while (i < used) {
        char v = sequence[i++];
        if (v >= '0' && v <= '9')
            a[n] = a[n] * 10 + (unsigned)(v - '0');
        else if (v == ';' && n < 15)
            ++n;
    }
    if (end == 'H') {
        row = a[0] ? a[0] - 1 : 0;
        col = a[1] ? a[1] - 1 : 0;
    } else if (end == 'G')
        col = a[0] ? a[0] - 1 : 0;
    else if(end=='A'){unsigned n=a[0]?a[0]:1;row=row>n?row-n:0;}
    else if(end=='B'){unsigned n=a[0]?a[0]:1;row=row+n<H?row+n:H-1;}
    else if (end == 'C')
        col += a[0] ? a[0] : 1;
    else if (end == 'D') {
        unsigned amount = a[0] ? a[0] : 1;
        col = col > amount ? col - amount : 0;
    } else if (end == 's') {
        saved_row = row; saved_col = col;
    } else if (end == 'u') {
        row = saved_row; col = saved_col;
    }
    else if (end == 'J') {
        for (r = 0; r < H; ++r)
            for (c = 0; c < W; ++c)
                if (a[0] == 2 || r > row || (r == row && c >= col))
                    blank(r, c);
    } else if (end == 'K') {
        if (row < H)
            for (c = 0; c < W; ++c)
                if (a[0] == 2 || c >= col)
                    blank(row, c);
    } else if (end == 'm') {
        for (i = 0; i <= n; ++i) {
            if (a[i] == 0) {
#ifdef PS_RICH_REPLAY
                attributes=0;
#endif
                fg = default_fg;
                bg = default_bg;
            } else if (a[i] == 7) {
                unsigned t = fg;
                fg = bg;
                bg = t;
            } else if (a[i] >= 100 && a[i] <= 107) {
                bg=palette(a[i]-100+8);
#ifdef PS_RICH_REPLAY
            } else if(a[i]==1||a[i]==2||a[i]==3||a[i]==4||a[i]==9){
                attributes|=1u<<a[i];
#endif
            } else if (a[i] >= 90 && a[i] <= 97) {
                fg = palette(a[i] - 90 + 8);
            } else if (a[i] >= 30 && a[i] <= 37) {
                fg = palette(a[i] - 30);
            } else if (a[i] >= 40 && a[i] <= 47) {
                bg = palette(a[i] - 40);
            } else if ((a[i] == 38 || a[i] == 48) && i + 2 <= n && a[i + 1] == 5) {
                if (a[i] == 38)
                    fg = palette(a[i + 2]);
                else
                    bg = palette(a[i + 2]);
                i += 2;
            } else if ((a[i] == 38 || a[i] == 48) && i + 4 <= n && a[i + 1] == 2) {
                unsigned rgb = (a[i + 2] << 16) | (a[i + 3] << 8) | a[i + 4];
                if (a[i] == 38)
                    fg = rgb;
                else
                    bg = rgb;
                i += 4;
            }
        }
    }
}
static void put(const char *p) {
    unsigned i, span = (unsigned char)p[0] == 0xf0 && (unsigned char)p[1] == 0x9f ? 2 : 1;
    #ifdef PS_RICH_REPLAY
    uint32_t cp;ps_rich_decode(p,ps_text_length(p),&cp);span=ps_rich_cell_width(cp);if(!span)return;
#endif
    if (row < H && col < W) {
#ifdef PS_RICH_REPLAY
        screen[row][col].attributes=attributes;
#endif
        for (i = 0; p[i] && i < 4; ++i)
            screen[row][col].text[i] = p[i];
        screen[row][col].text[i] = 0;
        screen[row][col].span = span;
        if(span==2 && col+1<W){blank(row,col+1);screen[row][col+1].span=0;}
        screen[row][col].fg = fg;
        screen[row][col].bg = bg;
    }
    col+=span;
}
static void sink(void *ctx, const unsigned char *p, unsigned count) {
    (void)ctx;
    while (count--) {
        unsigned c = *p++;
        if (escape == 1) {
            escape = c == '[' ? 2 : 0;
            used = 0;
            continue;
        }
        if (escape == 2) {
            if (c >= 64 && c <= 126) {
                csi((char)c);
                escape = 0;
            } else if (used + 1 < sizeof sequence)
                sequence[used++] = (char)c;
            continue;
        }
        if (c == 27) {
            escape = 1;
            continue;
        }
        if (c == 13) {
            col = 0;
            continue;
        }
        if (c == 8) { if (col) --col; continue; }
        if (c == 10) {
            ++row;
            continue;
        }
        if (c < 32)
            continue;
        if (utf_left) {
            utf[utf_n++] = (char)c;
            if (!--utf_left) {
                utf[utf_n] = 0;
                put(utf);
            }
            continue;
        }
        if (c >= 192) {
            utf[0] = (char)c;
            utf_n = 1;
            utf_left = c >= 240 ? 3 : c >= 224 ? 2 : 1;
            continue;
        }
        {
            char v[2] = {(char)c, 0};
            put(v);
        }
    }
}
static char *dst;
static unsigned size;
static void add(const char *p) {
    while (*p && size < 399990)
        dst[size++] = *p++;
}
static void number(unsigned n) {
    char b[12];
    unsigned i = 0;
    do {
        b[i++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (i) {
        char x[2] = {b[--i], 0};
        add(x);
    }
}
static void hex(unsigned n) {
    unsigned i;
    char b[7];
    for (i = 0; i < 6; ++i)
        b[i] = "0123456789abcdef"[(n >> (20 - i * 4)) & 15];
    b[6] = 0;
    add(b);
}
/* Terminal emulators commonly draw box cells themselves to keep strokes joined
   at arbitrary line heights. Mirror that cell behavior in the replay utility. */
static int box(Cell *v, unsigned x, unsigned y) {
    unsigned c;
    if ((unsigned char)v->text[0] != 0xe2 ||
        ((unsigned char)v->text[1] != 0x94 && (unsigned char)v->text[1] != 0x95))
        return 0;
    c = (unsigned char)v->text[2];
    if ((unsigned char)v->text[1] == 0x95 ? (c < 0xad || c > 0xb0)
                                          : (c != 0x80 && c != 0x81 && c != 0x82 && c != 0x9c &&
                                             c != 0xa4 && c != 0xac && c != 0xb4 && c != 0xbc))
        return 0;
    add("<path fill=\"none\" stroke=\"#");
    hex(v->fg);
    add(c == 0x81 ? "\" stroke-width=\"3\" d=\"M " : "\" stroke-width=\"1.25\" d=\"M ");
    if (c == 0x80 || c == 0x81) {
        number(x);
        add(" ");
        number(y + 13);
        add(" h 12");
    } else if (c == 0x9c || c == 0xa4 || c == 0xac || c == 0xb4 || c == 0xbc) {
        number(x + (c == 0x9c ? 6 : 0));
        add(" ");
        number(y + 13);
        add(" h ");
        number(c == 0x9c || c == 0xa4 ? 6 : 12);
        add(" M ");
        number(x + 6);
        add(" ");
        number(y + (c == 0xac ? 13 : 0));
        add(" v ");
        number(c == 0xac || c == 0xb4 ? 13 : 26);
    } else if (c == 0x82) {
        number(x + 6);
        add(" ");
        number(y);
        add(" v 26");
    } else if (c == 0xad) {
        number(x + 6);
        add(" ");
        number(y + 26);
        add(" v -7 q 0 -6 6 -6");
    } else if (c == 0xae) {
        number(x);
        add(" ");
        number(y + 13);
        add(" q 6 0 6 6 v 7");
    } else if (c == 0xb0) {
        number(x + 6);
        add(" ");
        number(y);
        add(" v 7 q 0 6 6 6");
    } else {
        number(x + 6);
        add(" ");
        number(y);
        add(" v 7 q 0 6 -6 6");
    }
    add("\"/>");
    return 1;
}
static unsigned svg(char *p) {
    unsigned r, c;
    dst = p;
    size = 0;
    add("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"");
    number(W * 12 + 64);
    add("\" height=\"");
    number(H * 26 + 64);
    add("\" viewBox=\"0 0 ");
    number(W * 12 + 64);
    add(" ");
    number(H * 26 + 64);
    add("\"><rect width=\"");
    number(W * 12 + 64);
    add("\" height=\"");
    number(H * 26 + 64);
    add("\" rx=\"18\" fill=\"#");
    hex(screen[0][0].bg);
    add("\"/><g font-family=\"Agave\" font-size=\"24\">");
    for (r = 0; r < H; ++r)
        for (c = 0; c < W; ++c) {
            Cell *v = &screen[r][c];
            if(!v->span)continue;
            if (v->bg != screen[0][0].bg) {
                add("<rect x=\"");
                number(32 + c * 12);
                add("\" y=\"");
                number(32 + r * 26);
                add(v->span==2?"\" width=\"24\" height=\"26\" fill=\"#":"\" width=\"12\" height=\"26\" fill=\"#");
                hex(v->bg);
                add("\"/>");
            }
            if (box(v, 32 + c * 12, 32 + r * 26))
                continue;
            if (v->text[0] && v->text[0] != ' ') {
                add("<text ");
#ifdef PS_RICH_REPLAY
                if(v->attributes&(1u<<1))add("font-weight=\"bold\" ");
                if(v->attributes&(1u<<2))add("opacity=\"0.6\" ");
                if(v->attributes&(1u<<3))add("font-style=\"italic\" ");
                if(v->attributes&(1u<<4))add("text-decoration=\"underline\" ");
                else if(v->attributes&(1u<<9))add("text-decoration=\"line-through\" ");
                if((unsigned char)v->text[0]>=0xe1&&(unsigned char)v->text[0]<=0xef)add("font-family=\"Noto Sans CJK JP\" ");
#endif
                if((unsigned char)v->text[0]==0xef && ((unsigned char)v->text[1]&0xc0)==0x80)add("font-family=\"Agave Nerd Font Mono\" ");
                if((unsigned char)v->text[0]==0xf0)add("font-family=\"Noto Emoji\" ");
                add("x=\"");
                number(32 + c * 12);
                add("\" y=\"");
                number(52 + r * 26);
                add("\" fill=\"#");
                hex(v->fg);
                add("\">");
                if (v->text[0] == '&')
                    add("&amp;");
                else if (v->text[0] == '<')
                    add("&lt;");
                else if (v->text[0] == '>')
                    add("&gt;");
                else
                    add(v->text);
                add("</text>");
            }
        }
    add("</g>");
#ifdef PS_REPLAY_CURSOR
    /* Optional static cursor for prompt fixtures, at the emitted terminal position. */
    add("<rect x=\""); number(32 + col * 12);
    add("\" y=\""); number(32 + row * 26);
    add("\" width=\"2\" height=\"26\" fill=\"#abb2bf\"/>");
#endif
    add("</svg>");
    return size;
}
#ifndef PS_REPLAY_ONLY
static void feed(ps_session *s, const char *p) {
    ps_feed(s, p, len(p));
}
#endif
/* Shared ANSI replay surface for the optional device-view regression pictures. */
void ps_replay_reset(void) {
#ifdef PS_RICH_REPLAY
    attributes=0;
#endif
    unsigned r,c;
    row=col=saved_row=saved_col=escape=used=utf_n=utf_left=0;fg=default_fg=0xabb2bf;bg=default_bg=0x282c34;
    for(r=0;r<H;++r)for(c=0;c<W;++c)blank(r,c);
}
void ps_replay_feed(const unsigned char *bytes,unsigned count) { sink(0,bytes,count); }
unsigned ps_replay_svg(char *buffer) { return svg(buffer); }
unsigned ps_replay_row(void){return row;}
unsigned ps_replay_column(void){return col;}
const char *ps_replay_cell(unsigned r,unsigned c){return r<H&&c<W?screen[r][c].text:"";}
#ifndef PS_REPLAY_ONLY
__attribute__((noinline, noipa)) int render_complete(void) {
    return 0;
}
char preview_menu[400000];unsigned preview_menu_n;
char preview_document[400000];unsigned preview_document_n;
int render_entry(void) {
    unsigned r, c;
    ps_config cfg = {sink, 0, W, H, PS_INTERACTIVE | PS_COLOR | PS_UNICODE | PS_TRUECOLOR};
    ps_session *s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
    for (r = 0; r < H; ++r)
        for (c = 0; c < W; ++c)
            blank(r, c);
    ps_start(s);
    ps_execute(s, "task add 'Build the portable core'");
    ps_execute(s, "task add 'Keep the original Lumen palette'");
    ps_execute(s, "task add 'Polish the Agave dark theme'");
    ps_execute(s, "task add 'Choose the next TUI to bring in'");
    ps_execute(s, "task add 'Try the new workspace'");
    ps_execute(s, "task done 1");
    ps_execute(s, "task done 2");
    ps_execute(s, "set transport byte-stream");
    ps_execute(s, "set target x86_64");
    ps_execute(s, "set accent violet-magenta");
    ps_execute(s, "set font Agave");
    ps_execute(s, "set checkpoint human-review");
    ps_execute(s, "board");
    ps_tick(s, 100);
    feed(s, "\033[B\033[B ");
    ps_feed(s, "", 0);
    preview_tasks_n = svg(preview_tasks);
    ps_execute(s, "theme red");
    ps_tick(s, 200);
    ps_feed(s, "", 0);
    preview_red_n = svg(preview_red);
    ps_execute(s, "theme lumen");
    ps_tick(s, 300);
    ps_feed(s, "", 0);
    preview_lumen_n = svg(preview_lumen);
    ps_execute(s, "theme semantic");
    ps_tick(s, 400);
    feed(s, "\r\005");
    ps_feed(s, "", 0);
    preview_edit_n = svg(preview_edit);
    feed(s, "\033");
    ps_tick(s, 500);
    feed(s, "\t");
    ps_feed(s, "", 0);
    preview_variables_n = svg(preview_variables);
    feed(s, "\r\025serial-console");
    ps_feed(s, "", 0);
    preview_value_edit_n = svg(preview_value_edit);
    ps_key(s, 27);
    ps_execute(s, "hash 'Portable bytes, useful tools' --rounds 10000");
    feed(s, "\t");
    for (r = 0; r < 29; ++r)
        ps_tick(s, 600 + r * 100);
    ps_feed(s, "", 0);
    preview_jobs_n = svg(preview_jobs);
    feed(s, "\t");
    ps_feed(s, "", 0);
    preview_session_n = svg(preview_session);
    ps_execute(s, "theme mono");
    ps_execute(s, "theme ascii");
    feed(s, "1");
    ps_feed(s, "", 0);
    preview_ascii_n = svg(preview_ascii);
    ps_execute(s, "theme semantic");
    ps_execute(s, "theme unicode");
    ps_execute(s, "theme color");
    ps_execute(s, "controls set 0 1");
    ps_execute(s, "controls set 3 2048");
    ps_execute(s, "controls set 4 3072");
    ps_execute(s, "controls set 5 192");
    ps_execute(s, "controls");
#if PS_CONTROL_SECTION_ROWS
    ps_key(s, PS_KEY_DOWN);
#endif
    ps_key(s, PS_KEY_DOWN);
    ps_key(s, PS_KEY_DOWN);
    ps_key(s, PS_KEY_DOWN);
    ps_tick(s, 4800);
    ps_feed(s, "", 0);
    preview_controls_n = svg(preview_controls);
    ps_set_table_style(s, PS_TABLE_DEFAULT | PS_TABLE_TABS_INSIDE);
    ps_feed(s, "", 0);
    preview_control_inside_n = svg(preview_control_inside);
    ps_set_table_style(s, PS_TABLE_DEFAULT | PS_TABLE_COMPACT_SECTIONS);
    ps_feed(s, "", 0);
    preview_control_compact_n = svg(preview_control_compact);
    ps_set_table_style(s, PS_TABLE_DEFAULT | PS_TABLE_CATEGORY_COLUMN);
    ps_tick(s, 4801);
    ps_feed(s, "", 0);
    ps_feed(s, "", 0);
    preview_control_columns_n = svg(preview_control_columns);
    ps_set_table_style(s, PS_TABLE_DEFAULT);
    ps_tick(s, 4802);
    ps_feed(s, "", 0);
    ps_execute(s, "theme onedark");
    ps_tick(s, 4900);
    ps_feed(s, "", 0);
    preview_onedark_n = svg(preview_onedark);
    ps_execute(s, "theme semantic");
    ps_tick(s, 5000);
    ps_key(s, 13);
    ps_tick(s, 5100);
    ps_feed(s, "", 0);
    preview_control_inline_n = svg(preview_control_inline);
    ps_key(s, 27);
    ps_key(s, PS_KEY_F2);
    ps_key(s, 21);
    feed(s, "2800");
    ps_tick(s, 5200);
    ps_feed(s, "", 0);
    preview_control_dialog_n = svg(preview_control_dialog);
    ps_key(s, 27);
#if PS_CONTROL_SECTION_ROWS
    ps_key(s, PS_KEY_DOWN);
    ps_key(s, PS_KEY_DOWN);
#endif
    ps_key(s, PS_KEY_DOWN);
    ps_key(s, PS_KEY_DOWN);
    ps_key(s, PS_KEY_DOWN);
    ps_key(s, 13);
    ps_tick(s, 5300);
    ps_feed(s, "", 0);
    preview_control_confirm_n = svg(preview_control_confirm);
    ps_key(s, 27);
    ps_key(s, 9);
    ps_tick(s, 5400);
    ps_feed(s, "", 0);
    preview_control_settings_n = svg(preview_control_settings);
    ps_key(s, 9);
#if PS_CONTROL_SECTION_ROWS
    ps_key(s, PS_KEY_UP);
    ps_key(s, ' ');
    ps_tick(s, 5401);
    ps_feed(s, "", 0);
    ps_feed(s, "", 0);
    preview_control_collapsed_n = svg(preview_control_collapsed);
#endif
    ps_execute(s, "theme ascii");
    ps_execute(s, "theme mono");
    ps_tick(s, 5500);
    ps_feed(s, "", 0);
    preview_control_ascii_n = svg(preview_control_ascii);
    {
        ps_device_profile profile;
        cfg.flags = PS_INTERACTIVE | PS_COLOR | PS_UNICODE | PS_CASE_INSENSITIVE | PS_NO_BUILTINS |
                    PS_LEGACY_KEYS;
        s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
        ps_device_attach(s, &profile, 0, 0);
        ps_clear(s);
        ps_device_greet(s, "Device", "fixture-host", "127.0.0.1");
        ps_start(s);
        feed(s, "EcHo  Quotes stay 'exact'\rhex -1\rsave\rget Sta\t");
        ps_message(s, PS_STYLE_WARNING, "Serial event fixture");
        preview_device_commands_n = svg(preview_device_commands);
    }
    ps_replay_reset();
    cfg.flags=PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_TRUECOLOR;
    s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    ps_execute(s,"menu");feed(s,"2\r3\r");
    preview_menu_n=svg(preview_menu);
    ps_replay_reset();s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);
    ps_editor_open(s,"Deployment notes", "Release checklist\n\nReview the device settings, verify the relay states, and confirm that every endpoint is reachable before applying the configuration. This paragraph wraps to the available terminal width without changing the saved text.\n\nNetwork\n\tHostname: workshop-controller\n\tTransport: serial or TCP\n\nFollow-up\n\tCheck the status page after saving.\n",0,0);
    ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_DOWN);ps_key(s,PS_KEY_END);feed(s,"now ");
    preview_document_n=svg(preview_document);
    return render_complete();
}
#endif
