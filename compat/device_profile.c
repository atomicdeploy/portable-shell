#include "portable_shell_device.h"
#include "session_settings.h"

static const char *const names[] = {"echo",  "clear",  "cls",     "exit",
                                    "hex",   "save",   "restore", "connect", "reboot", "busy",
                                    "ready", "status", "get",     "set",     "beep"};
static const char *const parameters[] = {"Hostname",         "WiFi_Mode",     "Station_SSID",
                                         "Station_Password", "AP_SSID",       "AP_Password",
                                         "Uptime",           "Firmware_Hash", "Build_Date", "input.uppercase"};
static unsigned lower(unsigned c) {
    return c >= 'A' && c <= 'Z' ? c + 32 : c;
}
static int starts(const char *text, const char *part) {
    while (*part)
        if (lower((unsigned char)*text++) != lower((unsigned char)*part++))
            return 0;
    return 1;
}
static unsigned complete(ps_session *s, int argc, const char *const *argv, const char *prefix,
                         const char **items, unsigned capacity, void *context) {
    unsigned i, n = 0;
    ps_device_slot *slot = context;
    (void)s;
    (void)argv;
    if (argc != 1 || (slot->command != 12 && slot->command != 13))
        return 0;
    for (i = 0; i < 10 && n < capacity; ++i)
        if (starts(parameters[i], prefix))
            items[n++] = parameters[i];
    return n;
}
/* Keep donor's comma/space-tolerant decimal input; reject overflow rather than
   inheriting signed-overflow undefined behavior. Hex output uses even digits. */
static int hexadecimal(ps_session *s, const char *p) {
    unsigned value = 0, negative = 0, i = 0, n = 0;
    char digits[9], output[9];
    while (*p) {
        unsigned c = (unsigned char)*p++;
        if (c == ' ' || c == '\t' || c == ',')
            continue;
        if (c == '-' && !negative) {
            negative = 1;
            continue;
        }
        if (c < '0' || c > '9' || value > (2147483648u - (c - '0')) / 10u) {
            ps_message(s, PS_STYLE_ERROR, "Invalid number.");
            return 2;
        }
        value = value * 10 + c - '0';
    }
    if (!negative && value > 2147483647u) {
        ps_message(s, PS_STYLE_ERROR, "Invalid number.");
        return 2;
    }
    if (negative)
        value = 0u - value;
    do {
        digits[n++] = "0123456789ABCDEF"[value & 15];
        value >>= 4;
    } while (value);
    if (n & 1)
        digits[n++] = '0';
    while (n)
        output[i++] = digits[--n];
    output[i] = 0;
    ps_write(s, output);
    ps_newline(s);
    return 0;
}
static int run(ps_session *s, const char *tail, void *context) {
    ps_device_slot *slot = context;
    const char *setting = tail;
    while (*setting == ' ' || *setting == '\t') ++setting;
    if ((slot->command == 12 || slot->command == 13) &&
        starts(setting, "input.uppercase") &&
        (!setting[sizeof "input.uppercase" - 1] || setting[sizeof "input.uppercase" - 1] == ' ' || setting[sizeof "input.uppercase" - 1] == '\t')) {
        ps_args args;
        if (ps_parse(setting, &args) ||
            args.argc != (slot->command == 13 ? 2 : 1)) {
            ps_message(s, PS_STYLE_ERROR, "set input.uppercase BOOL | get input.uppercase");
            return 2;
        }
        return session_uppercase_setting(s, slot->command == 13 ? args.argv[1] : 0);
    }
    switch (slot->command) {
    case 0:
        ps_write(s, tail);
        ps_newline(s);
        return 0;
    case 1:
    case 2:
        ps_clear(s);
        return 0;
    case 3:
        ps_request_exit(s);
        return 0;
    case 4:
        return hexadecimal(s, tail);
    default:
        if (slot->owner->call)
            return slot->owner->call(s, names[slot->command], tail, slot->owner->context);
        ps_message(s, PS_STYLE_ERROR, "Device backend not attached.");
        return 2;
    }
}
int ps_device_attach(ps_session *s, ps_device_profile *p, ps_device_call_fn call, void *context) {
    unsigned i;
    if (!s || !p)
        return -1;
    p->call = call;
    p->context = context;
    for (i = 0; i < sizeof names / sizeof names[0]; ++i) {
        ps_command command;
        p->slots[i].owner = p;
        p->slots[i].command = i;
        command.name = names[i];
        command.usage = (i == 12 || i == 13) ? "PARAMETER [RAW VALUE]" : "[RAW ARGUMENTS]";
        command.category = i < 5 ? "Shell" : "Device";
        command.run = 0;
        command.complete = complete;
        command.context = &p->slots[i];
        if (ps_register_raw(s, &command, run))
            return -1;
    }
    return ps_set_prompt(s, "> ", PS_STYLE_PROMPT);
}
static void append(char *out, unsigned *n, const char *text) {
    while (text && *text && *n < 255)
        out[(*n)++] = *text++;
    out[*n] = 0;
}
void ps_device_greet(ps_session *s, const char *product, const char *hostname, const char *peer) {
    char text[256];
    unsigned n = 0;
    append(text, &n, "Welcome ");
    if (peer && *peer) {
        append(text, &n, peer);
        append(text, &n, " ");
    }
    append(text, &n, "to ");
    append(text, &n, product);
    if (hostname && *hostname) {
        append(text, &n, " (");
        append(text, &n, hostname);
        append(text, &n, ")");
    }
    append(text, &n, "!");
    ps_message(s, PS_STYLE_SUCCESS, text);
}
