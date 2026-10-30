    if (s->editing && !s->control_view &&
        !(PS_WORKSPACE_INLINE_EDIT && (s->editing == 2 || s->editing == 3))) {
        const char *label = s->editing == 1   ? "New task"
                            : s->editing == 2 ? "Edit task"
                            : s->editing == 3 ? "Value"
                                              : "Filter";
        ink(s, 1);
        out(s, label);
        out(s, "  ");
        ink(s, 4);
        field(s, s->edit, w - length(label) - 6);
        reset(s);
        at(s, h - 3, 5 + length(label) + cells(s->edit, s->editpos, s->io.flags));
        ansi(s, "\033[?25h");
    } else {
