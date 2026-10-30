    if (key == PS_KEY_F2) {
        table_open(s, 0);
        return;
    }
    if (key == PS_KEY_F3) {
        table_open(s, 2);
        return;
    }
    if (key == PS_KEY_F1) {
        ps_newline(s);
        ps_execute(s, "help");
    } else if (key == PS_KEY_F4) {
#ifndef PS_ASCII_ONLY
        s->io.flags ^= PS_COLOR;
#endif
    } else
