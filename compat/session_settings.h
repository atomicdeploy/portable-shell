#ifndef PORTABLE_SESSION_SETTINGS_H
#define PORTABLE_SESSION_SETTINGS_H
/* Consumer-side setting vocabulary shared by the workbench and device profile. */
#include "portable_settings.h"
static int session_uppercase_setting(ps_session *s, const char *value) {
    ps_setting entry = {0};
    ps_setting_value parsed;
    if (!value) {
        ps_write(s, ps_get_uppercase_echo(s) ? "true" : "false");
        ps_newline(s);
        return 0;
    }
    entry.type = PS_SETTING_BOOL;
    if (ps_setting_parse(&entry, value, &parsed)) {
        ps_message(s, PS_STYLE_ERROR, "uppercase_echo requires true, false, 1 or 0");
        return 2;
    }
    ps_set_uppercase_echo(s, (int)parsed.natural);
    return 0;
}
#endif
