/* Small, purpose-built ANSI replay renderer for verification. No dependencies.
   This is a test utility, not the shell's terminal renderer. GDB exports SVG. */
#include "portable_shell.h"
#define W 100
#define H 28
typedef struct {
    char text[5];
    unsigned fg, bg;
} Cell;
static Cell screen[H][W];
static unsigned row, col, fg = 0xdce4ee, bg = 0x0c121d, escape, used;
static char sequence[64], utf[5];
static unsigned utf_n, utf_left;
static union {
    void *align;
    unsigned char bytes[40000];
} memory;
char preview_tasks[400000], preview_variables[400000], preview_jobs[400000],
    preview_session[400000], preview_edit[400000], preview_ascii[400000];
unsigned preview_tasks_n, preview_variables_n, preview_jobs_n, preview_session_n, preview_edit_n,
    preview_ascii_n;
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
                fg = 0xdce4ee;
                bg = 0x0c121d;
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
static unsigned svg(char *p) {
    unsigned r, c;
    dst = p;
    size = 0;
    add("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1060\" height=\"620\" viewBox=\"0 0 "
        "1060 620\"><rect width=\"1060\" height=\"620\" rx=\"18\" fill=\"#0c121d\"/><g "
        "font-family=\"Cascadia Code,Consolas,monospace\" font-size=\"16\">");
    for (r = 0; r < H; ++r)
        for (c = 0; c < W; ++c) {
            Cell *v = &screen[r][c];
            if (v->bg != 0x0c121d) {
                add("<rect x=\"");
                number(30 + c * 10);
                add("\" y=\"");
                number(27 + r * 20);
                add("\" width=\"10\" height=\"20\" fill=\"#");
                hex(v->bg);
                add("\"/>");
            }
            if (v->text[0] && v->text[0] != ' ') {
                add("<text x=\"");
                number(30 + c * 10);
                add("\" y=\"");
                number(43 + r * 20);
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
    ps_config cfg = {sink, 0, W, H, PS_INTERACTIVE | PS_COLOR | PS_UNICODE};
    ps_session *s = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
    for (r = 0; r < H; ++r)
        for (c = 0; c < W; ++c)
            blank(r, c);
    ps_start(s);
    ps_execute(s, "task add 'Compile the freestanding core'");
    ps_execute(s, "task add 'Exercise quoted arguments and completion'");
    ps_execute(s, "task add 'Review terminal rendering'");
    ps_execute(s, "task add 'Human interaction pass'");
    ps_execute(s, "task done 1");
    ps_execute(s, "task done 2");
    ps_execute(s, "set transport byte-stream");
    ps_execute(s, "set target x86_64");
    ps_execute(s, "set checkpoint human-review");
    ps_execute(s, "board");
    ps_tick(s, 100);
    feed(s, "\033[B\033[B ");
    preview_tasks_n = svg(preview_tasks);
    feed(s, "\r\005");
    preview_edit_n = svg(preview_edit);
    feed(s, "\033");
    ps_tick(s, 200);
    feed(s, "\t");
    preview_variables_n = svg(preview_variables);
    ps_execute(s, "hash 'Portable bytes, useful tools' --rounds 10000");
    feed(s, "\t");
    for (r = 0; r < 29; ++r)
        ps_tick(s, 300 + r * 100);
    preview_jobs_n = svg(preview_jobs);
    feed(s, "\t");
    preview_session_n = svg(preview_session);
    ps_execute(s, "theme mono");
    ps_execute(s, "theme ascii");
    feed(s, "1");
    preview_ascii_n = svg(preview_ascii);
    return render_complete();
}
