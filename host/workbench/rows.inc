static void task_row(ps_session *s, unsigned index, const Grid *g) {
    Task *t = &s->tasks[index];
    char b[20];
    ink(s, 1);
    str(b, t->selected ? ((s->io.flags & PS_UNICODE) ? "◆ " : "* ") : "  ", sizeof b);
    dec(b + length(b), t->id);
    field(s, b, g->widths[0]);
    grid_divide(s);
    ink(s, t->done ? 5 : 6);
    field(s,
          (s->io.flags & PS_UNICODE) ? (t->done ? "✓ Done" : "○ Open")
                                     : (t->done ? "[x] Done" : "[ ] Open"),
          g->widths[1]);
    grid_divide(s);
    ink(s, t->done ? 0 : 4);
    grid_text(s, t->title, g->widths[2]);
}
static void pair_row(ps_session *s, const char *key, const char *value, const Grid *g) {
    ink(s, 1);
    grid_text(s, key, g->widths[0]);
    grid_divide(s);
    ink(s, s->theme==PS_THEME_DEFAULT?PS_INK_STRING:2);
    grid_text(s, value, g->widths[1]);
}
static void stat_row(ps_session *s, unsigned i, const Grid *g) {
    char b[32];
    const char *key = "", *value = b;
    b[0] = 0;
    switch (i) {
    case 0:
        key = "Core";
        value = "PortableShell " PS_VERSION;
        break;
    case 1:
        key = "Session bytes";
        dec(b, sizeof *s);
        break;
    case 2:
        key = "Heap bytes";
        value = "0";
        break;
    case 3:
        key = "Input bytes";
        dec(b, s->bytes_in);
        break;
    case 4:
        key = "Commands run";
        dec(b, s->executions);
        break;
    case 5:
        key = "History entries";
        dec(b, s->history_count);
        break;
    case 6:
        key = "Transport";
        value = (s->io.flags & PS_TELNET) ? "Telnet / byte callbacks" : "Raw / byte callbacks";
        break;
    case 7:
        key = "Terminal";
        value = s->telnet.terminal;
        break;
    case 8:
        key = "Uptime seconds";
        dec(b, s->now / 1000);
        break;
    case 9:
        key = "Memory backend";
        value = ps_memory_backend();
        break;
    }
    pair_row(s, key, value, g);
}
static void progress_bar(ps_session *s, unsigned width_) {
    unsigned i, pct = s->job.total ? (unsigned)((s->job.done * 100u) / s->job.total) : 0;
    ink(s, 1);
    for (i = 0; i < width_; ++i)
        out(s, i * 100 / width_ < pct ? ((s->io.flags & PS_UNICODE) ? "━" : "#")
                                      : ((s->io.flags & PS_UNICODE) ? "─" : "."));
    out(s, " ");
    ps_number(s, pct);
    out(s, "%");
}
