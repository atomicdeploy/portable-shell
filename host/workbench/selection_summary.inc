    if (!s->control_view && s->filter[0] && s->tab == 0)
        out(s, " filtered");
    if (!s->control_view && s->tab == 0) {
        unsigned selected = 0;
        for (i = 0; i < s->task_count; ++i)
            selected += s->tasks[i].selected != 0;
        at(s, h - 5, w - 18);
        ink(s, 1);
        ps_number(s, selected);
        out(s, " selected");
    }
