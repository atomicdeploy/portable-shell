/* Standalone compile/symbol probe for the MCU profile; no hosted test runtime. */
#define PS_EDITOR_CAPACITY 64
#define PS_EDITOR_HISTORY 2
#define PS_EDITOR_PROMPT_BYTES 12
#define PS_EDITOR_SUGGEST_BYTES 24
#define PS_TERMINAL_SOFT_DIV10 1
#include "portable_editor.h"
#include "portable_parse.h"
#include "portable_probe.h"
static ps_editor editor;
static ps_terminal_probe probe;
static volatile unsigned emitted;
static void sink(void *ctx, const unsigned char *p, size_t n) {
    (void)ctx; while (n--) emitted += *p++;
}
void ps_micro_init(void) { ps_editor_init(&editor); editor.echo = 1; ps_probe_begin(&probe, 0, sink, 0); }
int ps_micro_feed(unsigned char ch, uint32_t now) {
    ps_probe_tick(&probe, now);
    if (probe.active) return ps_probe_feed(&probe, ch);
    return ps_editor_feed(&editor, ch, sink, 0);
}
int ps_micro_parse(char *buffer, size_t size, char **argv, unsigned limit) {
    ps_parse_result result;
    return ps_parse_tokens(buffer, size, buffer, size, argv, limit, &result);
}
