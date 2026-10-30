#define PS_EDITOR_CAPACITY 64
#define PS_EDITOR_HISTORY 2
#define PS_EDITOR_PROMPT_BYTES 12
#define PS_EDITOR_SUGGEST_BYTES 24
#define PS_TERMINAL_SOFT_DIV10 1
#include "portable_editor.h"
#include "portable_parse.h"
#include "portable_probe.h"
#include "portable_style.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static char output[2048];
static size_t used;
static void write_bytes(void *ctx, const unsigned char *p, size_t n) {
    (void)ctx; assert(used + n < sizeof output);
    memcpy(output + used, p, n); used += n; output[used] = 0;
}
int main(void) {
    {
        char grid[128],tiny[3]={'a','b','c'};
        ps_style_footer_format(grid,sizeof grid,"Enter apply  Esc cancel  Tab panel",80,1);
        assert(!strncmp(grid+20,"Esc cancel",10)&&!strcmp(grid+40,"Tab panel"));
        ps_style_footer_format(grid,sizeof grid,"Enter apply  Esc cancel",25,1);
        assert(!strcmp(grid,"Enter apply  Esc cancel"));
        ps_style_footer_format(grid,sizeof grid,"Enter apply  Esc cancel",80,0);
        assert(!strcmp(grid,"Enter apply  Esc cancel"));
        ps_style_footer_format(tiny,2,"Enter apply",80,1);
        assert(tiny[0]=='E'&&!tiny[1]&&tiny[2]=='c');
    }
    char buffer[80] = "cmd 'a b' \"\" a\\ b # comment";
    char *args[8]; ps_parse_result r; ps_terminal_probe probe;
    ps_editor editor; unsigned i; int event = 0;
    for (i = 0; i < 65536u; ++i) { unsigned remainder; assert(ps_div10(i, &remainder) == i / 10 && remainder == i % 10); }
    { unsigned remainder; assert(ps_div10(~0u, &remainder) == ~0u / 10 && remainder == ~0u % 10); }
    assert(!ps_parse_tokens(buffer, sizeof buffer, buffer, sizeof buffer, args, 8, &r));
    assert(r.argc == 4 && !strcmp(args[1], "a b") && !strcmp(args[2], "") && !strcmp(args[3], "a b"));
    assert(ps_parse_tokens("'x", 3, buffer, sizeof buffer, args, 8, &r) < 0);
    assert(ps_parse_tokens("abcdef", 4, buffer, sizeof buffer, args, 8, &r) < 0 && r.error_at == 4);
    assert(ps_parse_tokens("a b", 4, buffer, sizeof buffer, args, 1, &r) < 0);
    assert(ps_parse_tokens("abc", 4, buffer, 3, args, 8, &r) < 0);
    ps_editor_init(&editor); editor.echo = 1;
    for (i = 0; i < 65; ++i) ps_editor_feed(&editor, 'x', write_bytes, 0);
    assert(ps_editor_feed(&editor, '\r', write_bytes, 0) == PS_EDIT_TOO_LONG);
    ps_editor_clear(&editor);
    assert(ps_editor_feed(&editor, '\n', write_bytes, 0) == PS_EDIT_NONE);
    for (i = 0; i < 4; ++i) event = ps_editor_feed(&editor, (unsigned char)"led\r"[i], write_bytes, 0);
    assert(event == PS_EDIT_SUBMIT && ps_editor_remember(&editor, 1));
    ps_editor_clear(&editor);
    ps_editor_feed(&editor, 27, write_bytes, 0); ps_editor_feed(&editor, '[', write_bytes, 0);
    ps_editor_feed(&editor, 'A', write_bytes, 0); assert(!strcmp(editor.command, "led"));
    used = 0; ps_probe_begin(&probe, 0xfffffff0u, write_bytes, 0);
    assert(used == strlen("\033[s\033[999;999H\033[6n") && !memchr(output, 0, used));
    assert(ps_probe_tick(&probe, 20) == PS_PROBE_WAIT);
    for (i = 0; i < strlen("\033[123;240R"); ++i) event = ps_probe_feed(&probe, (unsigned char)"\033[123;240R"[i]);
    assert(event == PS_PROBE_READY && probe.row == 123 && probe.column == 240);
    ps_probe_restore(write_bytes, 0);
    ps_probe_begin(&probe, 0xfffffff0u, write_bytes, 0);
    assert(ps_probe_tick(&probe, 484) == PS_PROBE_TIMEOUT);
    ps_probe_begin(&probe, 0, write_bytes, 0);
    for (i = 0; i < strlen("\033[99999;1R"); ++i) event = ps_probe_feed(&probe, (unsigned char)"\033[99999;1R"[i]);
    assert(event == PS_PROBE_INVALID);
    ps_probe_begin(&probe, 0, write_bytes, 0);
    assert(ps_probe_feed(&probe, 'x') == PS_PROBE_INVALID && probe.bytes[0] == 'x');
    printf("micro editor=%u probe=%u bytes; bounded parser/probe/editor checks passed\n", (unsigned)sizeof editor, (unsigned)sizeof probe);
    return 0;
}
