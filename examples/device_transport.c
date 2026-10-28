/* Platform-neutral integration seam. No Arduino, ESPTelnet or OS headers.
   Compile with the same capacity definitions as the library. Instantiate once
   for serial and once per TCP client; each owns its prompt, history and output. */
#include "portable_shell_device.h"
typedef struct {
    union {
        void *alignment;
#if PS_LINE <= 128 && PS_HISTORY <= 4 && PS_ROWS <= 4 && PS_COMMANDS <= 24
        unsigned char bytes[8192];
#else
        unsigned char bytes[32768];
#endif
    } storage;
    ps_device_profile profile;
    ps_session *shell;
} device_channel;

/* tx must copy/consume every byte. Handle partial hardware writes in a bounded
   adapter queue, then drain it in the firmware's loop. Returns -1 on setup error.
   firmware receives canonical command names and untouched raw arguments. */
int device_channel_begin(device_channel *channel, ps_write_fn tx, void *tx_context, int telnet,
                        ps_device_call_fn firmware, void *firmware_context) {
    ps_config config = {tx, tx_context, 80, 24,
                        PS_INTERACTIVE | PS_COLOR | PS_CASE_INSENSITIVE | PS_NO_BUILTINS |
                            PS_LEGACY_KEYS};
    if (telnet)
        config.flags |= PS_TELNET;
    channel->shell = ps_init(channel->storage.bytes, sizeof channel->storage.bytes, &config);
    if (!channel->shell ||
        ps_device_attach(channel->shell, &channel->profile, firmware, firmware_context))
        return -1;
    /* Invoke only after the transport is writable, matching ESPTelnet's deferred
       greeting requirement. The optional ps_device_greet uses caller-owned facts. */
    ps_start(channel->shell);
    return 0;
}
int device_channel_service(device_channel *channel, const ps_byte *bytes, unsigned count,
                          ps_u32 monotonic_ms) {
    ps_feed(channel->shell, bytes, count);
    ps_tick(channel->shell, monotonic_ms);
    return ps_exited(channel->shell); /* caller closes socket / ends serial session */
}
void device_channel_event(device_channel *channel, const char *text) {
    ps_message(channel->shell, PS_STYLE_DEFAULT, text);
}
