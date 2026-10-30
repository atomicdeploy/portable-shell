#ifndef PORTABLE_WINDOWS_CONSOLE_H
#define PORTABLE_WINDOWS_CONSOLE_H
/* Host-only policy. The normal prompt must not claim terminal scrollback input. */
static DWORD ps_windows_input_mode(DWORD original, int mouse) {
    DWORD mode = (original & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT |
        ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT)) |
        ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    return mouse ? mode | ENABLE_MOUSE_INPUT : mode;
}
#endif
