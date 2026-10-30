static int add_task(ps_session *s, const char *p) {
    Task *t;
    if (!*p || length(p) >= PS_VALUE || s->task_count == PS_ROWS)
        return 0;
    t = &s->tasks[s->task_count++];
    zero(t, sizeof *t);
    str(t->title, p, PS_VALUE);
    t->id = s->next_id++;
    return 1;
}
static void table_key(ps_session *s, unsigned key) {
    unsigned n = tab_count(s), ids[PS_ROWS], idx = 0, i, any = 0;
    if (s->control_view) {
        control_key(s, key);
        return;
    }
    if (s->tab == 0) {
        visible(s, ids);
        if (n)
            idx = ids[s->selected < n ? s->selected : 0];
    }
    if (s->editing) {
        if (key == 27 || key == 3) {
            s->editing = 0;
            notice(s, "Cancelled");
        } else if (key == 13) {
            int ok = 1;
            if (s->editing == 1)
                ok = add_task(s, s->edit);
            else if (s->editing == 2 && n)
                str(s->tasks[idx].title, s->edit, PS_VALUE);
            else if (s->editing == 3 && n)
                str(s->vars[s->selected].value, s->edit, PS_VALUE);
            else if (s->editing == 4)
                str(s->filter, s->edit, sizeof s->filter);
            s->editing = 0;
            notice(s, ok ? "Saved" : "Task empty or capacity reached");
        } else if (key == PS_KEY_LEFT)
            s->editpos = prev(s->edit, s->editpos);
        else if (key == PS_KEY_RIGHT)
            s->editpos = next(s->edit, s->editpos);
        else if (key == PS_KEY_HOME || key == 1)
            s->editpos = 0;
        else if (key == PS_KEY_END || key == 5)
            s->editpos = s->editlen;
        else if (key == 21) {
            s->edit[0] = 0;
            s->editpos = s->editlen = 0;
        } else if (key == 8 || key == 127)
            edit_delete(s->edit, &s->editlen, &s->editpos, 1);
        else if (key == PS_KEY_DELETE)
            edit_delete(s->edit, &s->editlen, &s->editpos, 0);
        else if (key >= 32 && key < 256)
            edit_insert(s->edit, &s->editlen, &s->editpos, PS_VALUE, key);
        s->dirty = 1;
        return;
    }
    if (key == 27 || key == 3 || key == 'q') {
        table_close(s);
        return;
    }
    if (key == 9 || key == PS_KEY_RIGHT) {
        s->tab = (s->tab + 1) % 4;
        s->selected = s->scroll = 0;
    } else if (key == PS_KEY_BACKTAB || key == PS_KEY_LEFT) {
        s->tab = (s->tab + 3) % 4;
        s->selected = s->scroll = 0;
    } else if (key >= '1' && key <= '4') {
        s->tab = key - '1';
        s->selected = s->scroll = 0;
    } else if (key == PS_KEY_DOWN || key == 'j') {
        if (s->selected + 1 < n)
            ++s->selected;
    } else if (key == PS_KEY_UP || key == 'k') {
        if (s->selected)
            --s->selected;
    } else if (key == PS_KEY_HOME)
        s->selected = 0;
    else if (key == PS_KEY_END)
        s->selected = n ? n - 1 : 0;
    else if (key == PS_KEY_PAGEDOWN)
        s->selected = min(s->selected + table_slots(s), n ? n - 1 : 0);
    else if (key == PS_KEY_PAGEUP)
        s->selected = s->selected > table_slots(s)
                          ? s->selected - table_slots(s)
                          : 0;
    else if (s->tab == 0) {
        if (key == 'n')
            enter_edit(s, 1, "");
        else if (key == '/')
            enter_edit(s, 4, s->filter);
        else if (key == 13 && n)
            enter_edit(s, 2, s->tasks[idx].title);
        else if (key == ' ' && n)
            s->tasks[idx].selected ^= 1;
        else if (key == 'd' && n) {
            for (i = 0; i < s->task_count; ++i)
                if (s->tasks[i].selected) {
                    s->tasks[i].done ^= 1;
                    any = 1;
                }
            if (!any)
                s->tasks[idx].done ^= 1;
            notice(s, "Task state updated");
        } else if (key == PS_KEY_DELETE && n) {
            move(&s->tasks[idx], &s->tasks[idx + 1], (s->task_count - idx - 1) * sizeof(Task));
            --s->task_count;
            notice(s, "Task removed");
        }
    } else if (s->tab == 1 && n) {
        if (key == 13)
            enter_edit(s, 3, s->vars[s->selected].value);
        else if (key == PS_KEY_DELETE) {
            move(&s->vars[s->selected], &s->vars[s->selected + 1],
                 (s->var_count - s->selected - 1) * sizeof(Variable));
            --s->var_count;
            notice(s, "Variable removed");
        }
    } else if (s->tab == 2 && key == 'c') {
        s->job.active = 0;
        s->job.cancelled = 1;
        notice(s, "Job cancelled");
    }
    s->dirty = 1;
}
