    else if (PS_WORKSPACE_INLINE_EDIT && (s->editing == 2 || s->editing == 3) && count) {
        unsigned x = 4, start = 0, width = grid.widths[grid.count - 1];
        for (i = 0; i + 1 < grid.count; ++i)
            x += grid.widths[i] + 3;
        while (cells(s->edit + start, s->editpos - start, s->io.flags) >= width)
            start = next(s->edit, start);
        s->mouse_edit_x=x;s->mouse_edit_y=body+s->selected-s->scroll;
        s->mouse_edit_start=start;s->mouse_edit_width=width;
        at(s, body + s->selected - s->scroll, x);
        if (s->io.flags & PS_COLOR)
            surface(s, 1);
        else
            ansi(s, "\033[7m");
        field(s, s->edit + start, width);
        reset(s);
        at(s, body + s->selected - s->scroll,
           x + cells(s->edit + start, s->editpos - start, s->io.flags));
        ansi(s, "\033[?25h");
    } else if (s->editing) {
        const char *label = s->editing == 1   ? "New task"
                            : s->editing == 2 ? "Edit task"
                            : s->editing == 3 ? "Value"
                                              : "Filter";
        at(s, h - 3, min(w - 1, 5 + length(label) + cells(s->edit, s->editpos, s->io.flags)));
    } else {
        at(s, 3, 1);
    }
