    if (!s->control_view && s->tab == 0 && w >= 76) {
        unsigned done = 0;
        for (i = 0; i < s->task_count; ++i)
            done += s->tasks[i].done != 0;
        at(s, 1, w - 21);
        ink(s, 5);
        ps_number(s, done);
        out(s, "/");
        ps_number(s, s->task_count);
        out(s, " complete");
    }
