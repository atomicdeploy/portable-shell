/* Small, purpose-built ANSI replay renderer for verification. No dependencies.
   This is a test utility, not the shell's terminal renderer. GDB exports SVG. */
#include "portable_shell.h"
#define W 96
#define H 24
typedef struct {
    char text[5];
    unsigned fg, bg;
} Cell;
static Cell screen[H][W];
static unsigned row, col, fg = 0xf1e7ff, bg = 0x17131f, escape, used;
static char sequence[64], utf[5];
static unsigned utf_n, utf_left;
static union {
    void *align;
    unsigned char bytes[40000];
} memory;
char preview_tasks[400000], preview_variables[400000], preview_jobs[400000],
    preview_session[400000], preview_edit[400000], preview_ascii[400000], preview_red[400000],
    preview_lumen[400000];
unsigned preview_tasks_n, preview_variables_n, preview_jobs_n, preview_session_n, preview_edit_n,
    preview_ascii_n, preview_red_n, preview_lumen_n;
static unsigned len(const char *p) {
    unsigned n = 0;
    while (p[n])
        ++n;
    return n;
}
static void blank(unsigned r, unsigned c) {
    screen[r][c].text[0] = ' ';
    screen[r][c].text[1] = 0;
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
    } else if (end == 'C')
        col += a[0] ? a[0] : 1;
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
                fg = 0xf1e7ff;
                bg = 0x17131f;
            } else if (a[i] == 7) {
                unsigned t = fg;
                fg = bg;
                bg = t;
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
    unsigned i;
    if (row < H && col < W) {
        for (i = 0; p[i] && i < 4; ++i)
            screen[row][col].text[i] = p[i];
        screen[row][col].text[i] = 0;
        screen[row][col].fg = fg;
        screen[row][col].bg = bg;
    }
    ++col;
}
static void sink(void *ctx, const ps_byte *p, unsigned count) {
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
    if ((unsigned char)v->text[0] != 0xe2 || (unsigned char)v->text[1] != 0x94)
        return 0;
    c = (unsigned char)v->text[2];
    if (c != 0x80 && c != 0x81 && c != 0x82 && c != 0xad && c != 0xae && c != 0xaf && c != 0xb0)
        return 0;
    add("<path fill=\"none\" stroke=\"#");
    hex(v->fg);
    add(c == 0x81 ? "\" stroke-width=\"3\" d=\"M " : "\" stroke-width=\"1.25\" d=\"M ");
    if (c == 0x80 || c == 0x81) {
        number(x);
        add(" ");
        number(y + 13);
        add(" h 12");
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
    add("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1216\" height=\"688\" viewBox=\"0 0 "
        "1216 688\"><rect width=\"1216\" height=\"688\" rx=\"18\" fill=\"#");
    hex(screen[0][0].bg);
    add("\"/><g font-family=\"Agave\" font-size=\"24\">");
    for (r = 0; r < H; ++r)
        for (c = 0; c < W; ++c) {
            Cell *v = &screen[r][c];
            if (v->bg != screen[0][0].bg) {
                add("<rect x=\"");
                number(32 + c * 12);
                add("\" y=\"");
                number(32 + r * 26);
                add("\" width=\"12\" height=\"26\" fill=\"#");
                hex(v->bg);
                add("\"/>");
            }
            if (box(v, 32 + c * 12, 32 + r * 26))
                continue;
            if (v->text[0] && v->text[0] != ' ') {
                add("<text x=\"");
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
    add("</g></svg>");
    return size;
}
static void feed(ps_session *s, const char *p) {
    ps_feed(s, p, len(p));
}
__attribute__((noinline, noipa)) int render_complete(void) {
    return 0;
}
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
    preview_tasks_n = svg(preview_tasks);
    ps_execute(s, "theme red");
    ps_tick(s, 200);
    preview_red_n = svg(preview_red);
    ps_execute(s, "theme lumen");
    ps_tick(s, 300);
    preview_lumen_n = svg(preview_lumen);
    ps_execute(s, "theme violet");
    ps_tick(s, 400);
    feed(s, "\r\005");
    preview_edit_n = svg(preview_edit);
    feed(s, "\033");
    ps_tick(s, 500);
    feed(s, "\t");
    preview_variables_n = svg(preview_variables);
    ps_execute(s, "hash 'Portable bytes, useful tools' --rounds 10000");
    feed(s, "\t");
    for (r = 0; r < 29; ++r)
        ps_tick(s, 600 + r * 100);
    preview_jobs_n = svg(preview_jobs);
    feed(s, "\t");
    preview_session_n = svg(preview_session);
    ps_execute(s, "theme mono");
    ps_execute(s, "theme ascii");
    feed(s, "1");
    preview_ascii_n = svg(preview_ascii);
    return render_complete();
}
