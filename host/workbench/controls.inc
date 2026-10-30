/* Local, per-session memory bench. Integrators replace this entire panel. */
static int local_control_read(void *ctx, unsigned id, ps_control_value *v) {
    ps_session *s = (ps_session *)ctx;
    v->flags = PS_CONTROL_VALID;
    v->revision = s->local_revision;
    if (id < 7)
        v->number = s->local_values[id];
    else if (id == 7)
        str(v->text, s->local_label, sizeof v->text);
    else if (id == 8)
        str(v->text, "Session memory", sizeof v->text);
    else if (id == 10)
        v->number = s->theme;
    else if (id == 11)
        v->number = !!(s->table_style & PS_TABLE_GRID);
    return 1;
}
static int local_control_write(void *ctx, unsigned id, const ps_control_value *v, ps_u32 rev) {
    ps_session *s = (ps_session *)ctx;
    if (rev != s->local_revision)
        return 0;
    if (id < 7)
        s->local_values[id] = v->number;
    else if (id == 7)
        str(s->local_label, v->text, sizeof s->local_label);
    else if (id == 9) {
        unsigned i;
        for (i = 0; i < 6; ++i)
            s->local_values[i] = 0;
    } else if (id == 10) {
        s->theme = v->number;
        if (s->board) {
            reset(s);
            ansi(s, "\033[2J");
        }
    } else if (id == 11)
        s->table_style = (s->table_style & ~PS_TABLE_GRID) | (v->number ? PS_TABLE_GRID : 0);
    else
        return 0;
    ++s->local_revision;
    return 1;
}
static void control_local_attach(ps_session *s) {
    static const char *const modes[] = {"Manual", "Automatic", "Hold"};
    static const char *const themes[] = {"Violet", "Red", "Cyan", "One Dark", "Default"};
    static const ps_control outputs[] = {
        {0, "RELAYS", "Relay 1", PS_CONTROL_TOGGLE, 0, 1, 1, 0, 0},
        {1, "RELAYS", "Relay 2", PS_CONTROL_TOGGLE, 0, 1, 1, 0, 0},
        {2, "RELAYS", "Relay 3", PS_CONTROL_TOGGLE, 0, 1, 1, 0, 0},
        {3, "PWM", "Channel 0", PS_CONTROL_LEVEL, 0, 4095, 41, 0, 0},
        {4, "PWM", "Channel 1", PS_CONTROL_LEVEL, 0, 4095, 41, 0, 0},
        {5, "LIGHTING", "Brightness", PS_CONTROL_LEVEL, 0, 255, 1, 0, 0},
        {9, "ACTIONS", "All outputs off", PS_CONTROL_ACTION, 0, 0, 0, 0, 0}};
    static const ps_control settings[] = {
        {7, "IDENTITY", "Bench name", PS_CONTROL_TEXT, 0, 0, 0, 0, 0},
        {6, "POLICY", "Mode", PS_CONTROL_CHOICE, 0, 2, 1, modes, 3},
        {10, "APPEARANCE", "Theme", PS_CONTROL_CHOICE, 0, 4, 1, themes, 5},
        {11, "APPEARANCE", "Column dividers", PS_CONTROL_TOGGLE, 0, 1, 1, 0, 0},
        {8, "STORAGE", "Backend", PS_CONTROL_LABEL, 0, 0, 0, 0, 0}};
    static const ps_control_page pages[] = {{"Outputs", outputs, 7}, {"Settings", settings, 5}};
    if (!s->local_label[0])
        str(s->local_label, "Local bench", sizeof s->local_label);
    s->local_panel.title = "Local bench";
    s->local_panel.status = "Memory only";
    s->local_panel.pages = pages;
    s->local_panel.count = 2;
    s->local_panel.read = local_control_read;
    s->local_panel.write = local_control_write;
    s->local_panel.context = s;
    s->panel = &s->local_panel;
}
static int controls_command(ps_session *s, int argc, const char *const *argv) {
    unsigned p, i, id = 0;
    const ps_control *found = 0;
    ps_control_value v;
    char b[80];
    if (!s->panel)
        control_local_attach(s);
    if(argc==2&&equal(argv[1],"dashboard")){
        if(!ps_panel_set_layout(s,PS_PANEL_GRID))return 2;
        return ps_panel_open(s,0)?0:2;
    }
    if (argc == 1 || equal(argv[1], "open")) {
        if (argc > 3 || (argc == 3 && (!control_decimal(argv[2], &id) || !id)))
            return 2;
        if (argc == 3)
            --id;
        ps_panel_set_layout(s,PS_PANEL_TABS);
        return ps_panel_open(s, id) ? 0 : 2;
    }
    if (equal(argv[1], "list") && argc == 2) {
        for (p = 0; p < s->panel->count; ++p)
            for (i = 0; i < s->panel->pages[p].count; ++i) {
                const ps_control *c = &s->panel->pages[p].controls[i];
                control_read(s, c, &v);
                control_value_text(s, c, &v, b, sizeof b);
                ps_number(s, c->id);
                out(s, "\t");
                ps_write(s, s->panel->pages[p].title);
                out(s, "\t");
                ps_write(s, c->label);
                out(s, "\t");
                ps_write(s, b);
                if (v.flags & PS_CONTROL_PENDING)
                    out(s, " [pending]");
                ps_newline(s);
            }
        return 0;
    }
    if (argc < 3 || !control_decimal(argv[2], &id))
        return 2;
    for (p = 0; p < s->panel->count; ++p)
        for (i = 0; i < s->panel->pages[p].count; ++i)
            if (s->panel->pages[p].controls[i].id == id)
                found = &s->panel->pages[p].controls[i];
    if (!found)
        return 2;
    control_read(s, found, &v);
    if (equal(argv[1], "get") && argc == 3) {
        control_value_text(s, found, &v, b, sizeof b);
        ps_write(s, b);
        if (v.flags & PS_CONTROL_PENDING)
            out(s, " [pending]");
        ps_newline(s);
        return (v.flags & PS_CONTROL_VALID) ? 0 : 2;
    }
    if (equal(argv[1], "set") && argc == 4 && found->kind != PS_CONTROL_ACTION &&
        found->kind != PS_CONTROL_LABEL) {
        if (found->kind == PS_CONTROL_TEXT) {
            if (length(argv[3]) >= sizeof v.text)
                return 2;
            str(v.text, argv[3], sizeof v.text);
        } else if (!control_decimal(argv[3], &v.number))
            return 2;
        return control_commit(s, found, &v, v.revision) ? 0 : 2;
    }
    return 2;
}
