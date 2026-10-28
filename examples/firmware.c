#include "portable_shell.h"
/* Example integration, not a hardware driver. Board supplies these functions. */
extern void board_tx(const ps_byte *, unsigned);
extern int board_rx(ps_byte *);
extern ps_u32 board_millis(void);
static union {
    void *align;
    unsigned char bytes[32768];
} storage;
static void tx(void *context, const ps_byte *data, unsigned n) {
    (void)context;
    board_tx(data, n);
}
int portable_shell_firmware_main(void) {
    ps_config cfg = {tx, 0, 80, 24, PS_INTERACTIVE | PS_COLOR | PS_UNICODE};
    ps_session *s = ps_init(storage.bytes, sizeof storage.bytes, &cfg);
    ps_byte b;
    if (!s)
        return 1;
    ps_start(s);
    while (!ps_exited(s)) {
        while (board_rx(&b))
            ps_feed(s, &b, 1);
        ps_tick(s, board_millis());
    }
    ps_close(s);
    return 0;
}
