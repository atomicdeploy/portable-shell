/* Freestanding C tests exercise the very headers used by firmware. */
#include "portable_editor.h"
static unsigned writes;
static void discard(void *ctx, const unsigned char *p, size_t n) {
    (void)ctx;
    (void)p;
    writes += (unsigned)n;
}
int portable_terminal_checks(void) {
    ps_telnet telnet;
    ps_editor editor;
    unsigned char decoded;
    unsigned i;
    char safe[32], fmt[6];
    {
        ps_mouse_event event;
        if(!ps_mouse_parse("<32;120;40M",&event)||event.button!=32||event.column!=120||event.row!=40||event.release)return 11;
        if(!ps_mouse_parse("<0;1;1m",&event)||!event.release)return 12;
        if(ps_mouse_parse("<0;0;1M",&event)||ps_mouse_parse("<0;65536;1M",&event)||
           ps_mouse_parse("<0;;1M",&event)||ps_mouse_parse("<0;1;1Mx",&event))return 13;
    }
    const unsigned char malformed[] = {255, 250, 31, 0, 80, 255, 241, 'x', '\r', 255, 240};
    ps_telnet_init(&telnet, PS_TN_BINARY, 80, 24);
    writes = 0;
    ps_telnet_greet(&telnet, discard, 0);
    if (writes != 21)
        return 1;
    for (i = 0; i < sizeof(malformed); ++i)
        if (ps_telnet_feed(&telnet, malformed[i], discard, 0, &decoded) != PS_TN_NONE)
            return 2;
    if (ps_telnet_feed(&telnet, 's', discard, 0, &decoded) != PS_TN_DATA || decoded != 's')
        return 3;
    ps_terminal_text(safe, sizeof(safe), "A\033]52;c;private\aB\033Pprivate\033\\C\0337D", 100);
    if (!ps_text_equal(safe, "ABCD"))
        return 4;
    if (ps_history_safe("set MQTT_TOKEN hidden", 64) || ps_history_safe("get https://u:p@h", 64) ||
        !ps_history_safe("status", 64))
        return 5;
    if (ps_terminal_format(fmt, sizeof(fmt), "%02u:%s", 4u, "long") != 7 ||
        !ps_text_equal(fmt, "04:lo"))
        return 6;
    ps_editor_init(&editor);
    writes = 0;
    for (i = 0; i < 6; ++i)
        ps_editor_feed(&editor, (unsigned char)"hidden"[i], discard, 0);
    if (writes || ps_editor_remember(&editor, 1))
        return 7;
    ps_editor_reset(&editor);
    for (i = 0; i < sizeof(editor.command); ++i)
        if (editor.command[i])
            return 8;
    editor.echo = 1;
    for (i = 0; i < PS_EDITOR_CAPACITY + 1; ++i)
        ps_editor_feed(&editor, 'a', discard, 0);
    if (ps_editor_feed(&editor, '\r', discard, 0) != PS_EDIT_TOO_LONG)
        return 9;
#if PS_STYLE_THEMES
    /* Original six Cyan inks must survive palette extraction/theme additions. */
    {
        static const char *const original[] = {"\033[38;5;245m", "\033[38;5;80m",
            "\033[38;5;223m", "\033[38;5;203m", "\033[1;38;5;231m", "\033[38;5;151m"};
        for (i = 0; i < 6; ++i)
            if (!ps_text_equal(ps_style_ink(2, 1, 1, i), original[i]) ||
                !ps_text_equal(ps_style_ink(2, 0, 1, i), original[i])) return 10;
    }
#endif
    return 0;
}
