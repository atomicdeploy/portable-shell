/* Private implementation: shared renderer/editor primitives, no OS/runtime imports. */
static int control_sections(const ps_session *s) {
    return PS_CONTROL_SECTION_ROWS && !(s->table_style & PS_TABLE_CATEGORY_COLUMN);
}
/* Encoded presentation rows keep provider indices stable. Headings never write. */
static unsigned control_items(ps_session *s, unsigned short *items, unsigned *focus, unsigned frame) {
    const ps_control_page *p = &s->panel->pages[s->tab];
    unsigned i, n = 0, closed = 0, start = 0;
    *focus = 0;
    for (i = 0; i < p->count; ++i) {
        const char *group = p->controls[i].group;
        if (control_sections(s) && (!i || !equal(group,p->controls[i-1].group))) {
            start = i;
            closed = *group && (s->control_closed[s->tab][i/8] & (1U << (i%8)));
            if (*group) {
                /* Reuse an adjacent rule (column header or closed section). */
                if (frame && !(s->table_style & PS_TABLE_COMPACT_SECTIONS) && n && !(items[n-1]&0x2000)) items[n++] = (unsigned short)(i | 0x4000);
                if (s->selected == i && s->control_header) *focus = n;
                items[n++] = (unsigned short)(i | 0x8000);
                if (frame && !(s->table_style & PS_TABLE_COMPACT_SECTIONS)) items[n++] = (unsigned short)(i | 0x2000);
            }
        }
        if (closed) {
            if (i == s->selected) { s->selected = start; s->control_header = 1; *focus = n-1-(frame && !(s->table_style & PS_TABLE_COMPACT_SECTIONS)); }
        } else {
            if (i == s->selected && !s->control_header) *focus = n;
            items[n++] = (unsigned short)i;
        }
    }
    return n;
}
static int control_navigate(ps_session *s, unsigned key, unsigned step) {
    unsigned short items[256]; unsigned focus, n;
    if (!control_sections(s)) return 0;
    n = control_items(s,items,&focus,0);
    if (!n) return 0;
    if (s->control_header && (key == 13 || key == ' ' || key == PS_KEY_LEFT || key == PS_KEY_RIGHT)) {
        ps_byte *bits = &s->control_closed[s->tab][s->selected/8];
        unsigned mask = 1U << (s->selected%8);
        if (key == PS_KEY_LEFT) *bits |= (ps_byte)mask;
        else if (key == PS_KEY_RIGHT) *bits &= (ps_byte)~mask;
        else *bits ^= (ps_byte)mask;
        return 1;
    }
    if (key == PS_KEY_DOWN || key == 'j') focus = min(focus+1,n-1);
    else if (key == PS_KEY_UP || key == 'k') focus = focus ? focus-1 : 0;
    else if (key == PS_KEY_PAGEDOWN) focus = min(focus+step,n-1);
    else if (key == PS_KEY_PAGEUP) focus = focus > step ? focus-step : 0;
    else return 0;
    s->selected = items[focus] & 0x7fff;
    s->control_header = (items[focus] & 0x8000) != 0;
    return 1;
}
static void control_section_row(ps_session *s, unsigned index, unsigned width) {
    char label[128]; unsigned i;
    const char *name = s->panel->pages[s->tab].controls[index].group;
    unsigned closed = s->control_closed[s->tab][index/8] & (1U << (index%8));
    for (i=0; name[i] && i+1<sizeof label; ++i)
        label[i] = name[i]>='a' && name[i]<='z' ? (char)(name[i]-'a'+'A') : name[i];
    label[i]=0;
    surface(s, s->control_header && s->selected == index ? 1 : 0);
    out(s,(s->io.flags & PS_UNICODE) ? (closed ? "▸ " : "▾ ") : (closed ? "+ " : "- "));
    grid_text(s,label,width-2);
}
static const ps_control *control_selected(ps_session *s) {
    const ps_control_page *p = &s->panel->pages[s->tab];
    return !s->control_header && s->selected < p->count ? &p->controls[s->selected] : 0;
}
static int control_read(ps_session *s, const ps_control *c, ps_control_value *v) {
    zero(v, sizeof *v);
    if (!s->panel->read(s->panel->context, c->id, v)) {
        zero(v, sizeof *v);
        return 0;
    }
    v->text[63] = 0;
    if (c->kind == PS_CONTROL_LEVEL && (v->number < c->minimum || v->number > c->maximum))
        v->flags &= ~PS_CONTROL_VALID;
    if ((c->kind == PS_CONTROL_TOGGLE && v->number > 1) ||
        (c->kind == PS_CONTROL_CHOICE && v->number >= c->choice_count))
        v->flags &= ~PS_CONTROL_VALID;
    return (v->flags & PS_CONTROL_VALID) != 0;
}
static int control_writable(ps_session *s, const ps_control *c, ps_control_value *v) {
    control_read(s, c, v);
    if (!(v->flags & PS_CONTROL_VALID)) {
        notice(s, "State unknown");
        return 0;
    }
    if (v->flags & PS_CONTROL_PENDING) {
        notice(s, "Waiting for readback");
        return 0;
    }
    if (!s->panel->write || (v->flags & PS_CONTROL_LOCKED) || c->kind == PS_CONTROL_LABEL) {
        notice(s, "Read only");
        return 0;
    }
    return 1;
}
static int control_decimal(const char *p, unsigned *n) {
    unsigned value = 0;
    if (!*p)
        return 0;
    while (*p) {
        if (*p < '0' || *p > '9' || value > 6553 || (value == 6553 && *p > '5'))
            return 0;
        value = value * 10 + (unsigned)(*p++ - '0');
    }
    *n = value;
    return 1;
}
static int control_validate(const ps_control *c, const ps_control_value *v) {
    unsigned i;
    if (c->kind == PS_CONTROL_TOGGLE)
        return v->number <= 1;
    if (c->kind == PS_CONTROL_LEVEL)
        return v->number >= c->minimum && v->number <= c->maximum;
    if (c->kind == PS_CONTROL_CHOICE)
        return v->number < c->choice_count;
    for (i = 0; i < sizeof v->text; ++i) {
        if (!v->text[i])
            return 1;
        if ((ps_byte)v->text[i] < 32 || (ps_byte)v->text[i] == 127)
            return 0;
    }
    return 0;
}
static int control_commit(ps_session *s, const ps_control *c, ps_control_value *request,
                          ps_u32 expected) {
    ps_control_value current;
    int result;
    if (!control_validate(c, request)) {
        notice(s, "Invalid value or outside range");
        return 0;
    }
    if (!control_writable(s, c, &current))
        return 0;
    if (current.revision != expected) {
        notice(s, "State changed; cancel and reopen editor");
        return 0;
    }
    result = s->panel->write(s->panel->context, c->id, request, expected);
    notice(s, result == 1 ? "Applied" : result == 2 ? "Pending readback" : "Rejected");
    s->dirty = 1;
    return result == 1 || result == 2;
}
static void control_value_text(ps_session *s, const ps_control *c, const ps_control_value *v,
                               char *text, unsigned cap) {
    if (!(v->flags & PS_CONTROL_VALID))
        str(text, "Unknown", cap);
    else if (c->kind == PS_CONTROL_TOGGLE)
        str(text,
            (s->io.flags & PS_UNICODE) ? (v->number ? "● On" : "○ Off")
                                       : (v->number ? "[x] On" : "[ ] Off"),
            cap);
    else if (c->kind == PS_CONTROL_CHOICE)
        str(text, c->choices[v->number], cap);
    else if (c->kind == PS_CONTROL_LEVEL)
        dec(text, v->number);
    else if (c->kind == PS_CONTROL_ACTION)
        str(text, "Run...", cap);
    else
        str(text, v->text, cap);
}
static void control_row(ps_session *s, unsigned index, const Grid *g) {
    const ps_control *c = &s->panel->pages[s->tab].controls[index];
    ps_control_value v;
    unsigned col = 0, w, i, pct;
    char b[80];
    control_read(s, c, &v);
    if (g->count == 3) {
        ink(s, 0);
        const ps_control_page *page = &s->panel->pages[s->tab];
        grid_text(s, index == s->scroll || !index || !equal(c->group, page->controls[index-1].group)
                         ? c->group : "", g->widths[col++]);
        grid_divide(s);
    }
    ink(s, 4);
    {
        unsigned marker=min(cells(s->selection_marker,length(s->selection_marker),s->io.flags),g->widths[col]);
        field(s,!s->control_header && s->selected==index?s->selection_marker:"",marker);
        grid_text(s,c->label,g->widths[col++]-marker);
    }
    grid_divide(s);
    w = g->widths[col];
    ink(s, !(v.flags & PS_CONTROL_VALID)  ? 0
           : c->kind == PS_CONTROL_TOGGLE ? (v.number ? PS_INK_SUCCESS : s->theme==PS_STYLE_SEMANTIC?PS_INK_INACTIVE:3)
           : c->kind == PS_CONTROL_ACTION ? PS_INK_ACTION
           : c->kind == PS_CONTROL_LEVEL ? PS_INK_NUMBER
           : c->kind == PS_CONTROL_CHOICE ? PS_INK_COMMAND
           : c->kind == PS_CONTROL_LABEL ? PS_INK_TEXT : PS_INK_STRING);
    if (s->editing == 5 && !s->dialog && index == s->selected) {
        unsigned start = 0;
        while (cells(s->edit + start, s->editpos - start, s->io.flags) >= w - 2)
            start = next(s->edit, start);
        surface(s, 1);
        out(s, " ");
        field(s, s->edit + start, w - 2);
        out(s, " ");
        return;
    }
    if ((v.flags & PS_CONTROL_VALID) && !(v.flags & (PS_CONTROL_PENDING | PS_CONTROL_LOCKED)) &&
        c->kind == PS_CONTROL_LEVEL && w >= 18) {
        unsigned bar = w - 15;
        pct = (v.number - c->minimum) * 100 / (c->maximum - c->minimum);
        for (i = 0; i < bar; ++i) {
            ink(s, i * 100 < pct * bar ? PS_INK_NUMBER : PS_INK_MUTED);
            out(s, (s->io.flags & PS_UNICODE) ? (i * 100 < pct * bar ? "━" : "─")
                                              : (i * 100 < pct * bar ? "#" : "."));
        }
        ink(s, PS_INK_NUMBER);
        out(s, " ");
        dec(b, pct);
        str(b + length(b), "%", sizeof b - length(b));
        field(s, b, 5);
        dec(b, v.number);
        str(b + length(b), "/", sizeof b - length(b));
        dec(b + length(b), c->maximum);
        field(s, b, 9);
    } else {
        control_value_text(s, c, &v, b, sizeof b);
        if (v.flags & PS_CONTROL_PENDING) {
            ink(s, s->theme==PS_STYLE_SEMANTIC?PS_INK_WARNING:6);
            str(b + length(b), " [pending]", sizeof b - length(b));
        } else if (v.flags & PS_CONTROL_LOCKED)
            str(b + length(b), " [locked]", sizeof b - length(b));
        grid_text(s, b, w);
    }
}
static const char *control_hint(ps_session *s) {
    const ps_control *c = control_selected(s);
    ps_control_value v;
    if (s->control_header)
        return (s->io.flags & PS_UNICODE) ? "Enter/Space toggle  ← collapse  → expand"
                                         : "Enter/Space toggle  Left close Right open";
    if (s->editing)
        return s->dialog ? "Tab focus   Enter apply   Esc cancel"
                         : "Enter apply  Esc cancel  ^U clear";
    if (!c)
        return "";
    control_read(s, c, &v);
    if (!(v.flags & PS_CONTROL_VALID))
        return "Waiting for state";
    if (v.flags & PS_CONTROL_PENDING)
        return "Waiting for readback";
    if ((v.flags & PS_CONTROL_LOCKED) || !s->panel->write)
        return "Read only";
    if (c->kind == PS_CONTROL_LEVEL)
        return (s->io.flags & PS_UNICODE) ? "←/→ adjust  Enter value  F2 dialog"
                                          : "Left/Right adjust  Enter value  F2 dialog";
    if (c->kind == PS_CONTROL_TOGGLE)
        return s->io.columns < 60 ? "Space toggle  Enter toggle  F2 edit"
                                  : "Space toggle   Enter toggle   F2 dialog";
    if (c->kind == PS_CONTROL_CHOICE)
        return (s->io.flags & PS_UNICODE) ? "←/→ choose   F2 dialog"
                                          : "Left/Right choose   F2 dialog";
    if (c->kind == PS_CONTROL_ACTION)
        return "Enter open confirmation";
    if (c->kind == PS_CONTROL_TEXT)
        return "Enter inline edit   F2 dialog";
    return "Read only";
}
static void control_overlay(ps_session *s, const Grid *table) {
    const ps_control *c = control_selected(s);
    unsigned w, x, y, i, start = 0, cursor_x, cursor_y;
    ps_control_value v;
    char b[80];
    if (!c || !s->editing)
        return;
    if (!s->dialog) {
        w = table->widths[table->count - 1];
        x = 4;
        for (i = 0; i + 1 < table->count; ++i)
            x += table->widths[i] + 3;
        while (cells(s->edit + start, s->editpos - start, s->io.flags) >= w - 2)
            start = next(s->edit, start);
        s->mouse_edit_x=x+1;s->mouse_edit_y=s->control_edit_row;
        s->mouse_edit_start=start;s->mouse_edit_width=w-2;
        at(s, s->control_edit_row,
           x + 1 + cells(s->edit + start, s->editpos - start, s->io.flags));
        ansi(s, "\033[?25h");
        return;
    }
    w = min(60, s->io.columns - 6);
    x = (s->io.columns - w) / 2 + 1;
    y = (s->io.rows - 9) / 2 + 1;
    for (i = 0; i < 9; ++i) {
        at(s, y + i, x);
        surface(s, 2);
        ink(s, 7);
        out(s, (s->io.flags & PS_UNICODE) ? (i == 0 ? "╭" : i == 8 ? "╰" : "│") : "+");
        if (i == 0 || i == 8) {
            unsigned j;
            for (j = 0; j < w - 2; ++j)
                out(s, (s->io.flags & PS_UNICODE) ? "─" : "-");
        } else
            spaces(s, w - 2);
        out(s, (s->io.flags & PS_UNICODE) ? (i == 0 ? "╮" : i == 8 ? "╯" : "│") : "+");
    }
    at(s, y + 1, x + 3);
    ink(s, 1);
    grid_text(s, c->label, w - 6);
    at(s, y + 2, x + 3);
    ink(s, 0);
    grid_text(s, c->group, w - 6);
    control_read(s, c, &v);
    control_value_text(s, c, &v, b, sizeof b);
    at(s, y + 3, x + 3);
    ink(s, 4);
    out(s, "Observed  ");
    grid_text(s, b, w - 16);
    at(s, y + 4, x + 3);
    if (c->kind == PS_CONTROL_ACTION) {
        ink(s, 6);
        grid_text(s, "Confirm action?", w - 6);
    } else {
        ink(s, 4);
        out(s, "Draft     ");
        while (cells(s->edit + start, s->editpos - start, s->io.flags) >= w - 17)
            start = next(s->edit, start);
        if (!s->dialog_focus)
            surface(s, 1);
        field(s, s->edit + start, w - 17);
        surface(s, 2);
    }
    cursor_x = x + 13 + cells(s->edit + start, s->editpos - start, s->io.flags);
    cursor_y = y + 4;
    s->mouse_edit_x=x+13;s->mouse_edit_y=cursor_y;
    s->mouse_edit_start=start;s->mouse_edit_width=w-17;
    at(s, y + 5, x + 3);
    ink(s, 0);
    if (c->kind == PS_CONTROL_LEVEL) {
        ps_number(s, c->minimum);
        out(s, "..");
        ps_number(s, c->maximum);
        out(s, "  step ");
        ps_number(s, c->step);
    } else if (c->kind == PS_CONTROL_CHOICE) {
        unsigned choice;
        if (control_decimal(s->edit, &choice) && choice < c->choice_count)
            grid_text(s, c->choices[choice], w - 6);
    } else if (c->kind == PS_CONTROL_TOGGLE)
        out(s, "0 Off   1 On");
    at(s, y + 7, x + 3);
    if (s->dialog_focus == 1)
        surface(s, 1);
    ink(s, 5);
    out(s, s->dialog_focus == 1 ? "[ Apply ]" : "  Apply  ");
    surface(s, 2);
    out(s, "   ");
    if (s->dialog_focus == 2)
        surface(s, 1);
    ink(s, 4);
    out(s, s->dialog_focus == 2 ? "[ Cancel ]" : "  Cancel  ");
    reset(s);
    if (!s->dialog_focus && c->kind != PS_CONTROL_ACTION) {
        at(s, cursor_y, cursor_x);
        ansi(s, "\033[?25h");
    }
}
static void control_begin(ps_session *s, const ps_control *c, unsigned dialog) {
    ps_control_value v;
    char b[64];
    if (!control_writable(s, c, &v))
        return;
    if (c->kind == PS_CONTROL_TEXT)
        str(b, v.text, sizeof b);
    else
        dec(b, v.number);
    enter_edit(s, 5, b);
    s->draft_revision = v.revision;
    s->notice[0] = 0;
    s->dialog = dialog || c->kind == PS_CONTROL_ACTION;
    s->dialog_focus = c->kind == PS_CONTROL_ACTION ? 2 : 0;
}
static void control_key(ps_session *s, unsigned key) {
    const ps_control *c = control_selected(s);
    unsigned n = tab_count(s), step = table_slots(s);
    ps_control_value v;
    s->dirty = 1;
    if (s->editing && c) {
        if (key == 27 || key == 3) {
            s->editing = s->dialog = 0;
            notice(s, "Cancelled");
        } else if (s->dialog && (key == 9 || key == PS_KEY_BACKTAB)) {
            unsigned first = c->kind == PS_CONTROL_ACTION ? 1 : 0;
            s->dialog_focus = key == 9 ? (s->dialog_focus == 2 ? first : s->dialog_focus + 1)
                                       : (s->dialog_focus == first ? 2 : s->dialog_focus - 1);
        } else if (key == 13) {
            if (s->dialog && s->dialog_focus == 2) {
                s->editing = s->dialog = 0;
                notice(s, "Cancelled");
                return;
            }
            zero(&v, sizeof v);
            if (c->kind == PS_CONTROL_TEXT)
                str(v.text, s->edit, sizeof v.text);
            else if (c->kind != PS_CONTROL_ACTION && !control_decimal(s->edit, &v.number)) {
                notice(s, "Enter a whole number in range");
                return;
            }
            if (control_commit(s, c, &v, s->draft_revision))
                s->editing = s->dialog = 0;
        } else if (s->dialog_focus && (key == PS_KEY_LEFT || key == PS_KEY_RIGHT)) {
            s->dialog_focus = s->dialog_focus == 1 ? 2 : 1;
        } else if (!s->dialog_focus) {
            if (key == PS_KEY_LEFT)
                s->editpos = prev(s->edit, s->editpos);
            else if (key == PS_KEY_RIGHT)
                s->editpos = next(s->edit, s->editpos);
            else if (key == PS_KEY_HOME || key == 1)
                s->editpos = 0;
            else if (key == PS_KEY_END || key == 5)
                s->editpos = s->editlen;
            else if (key == 21)
                s->edit[0] = 0, s->editpos = s->editlen = 0;
            else if (key == 8 || key == 127)
                edit_delete(s->edit, &s->editlen, &s->editpos, 1);
            else if (key == PS_KEY_DELETE)
                edit_delete(s->edit, &s->editlen, &s->editpos, 0);
            else if (key >= 32 && key < 256)
                edit_insert(s->edit, &s->editlen, &s->editpos, 64, key);
        }
        return;
    }
    if (key == 27 || key == 3 || key == 'q') {
        table_close(s);
        return;
    }
    if (key == 9 || key == PS_KEY_BACKTAB || (key >= '1' && key < '1' + s->panel->count)) {
        s->tab = key == 9                ? (s->tab + 1) % s->panel->count
                 : key == PS_KEY_BACKTAB ? (s->tab + s->panel->count - 1) % s->panel->count
                                         : key - '1';
        s->selected = s->scroll = 0;
        s->control_header = 0;
    } else if (control_navigate(s,key,step)) {
        return;
    } else if (key == PS_KEY_DOWN || key == 'j')
        s->selected = min(s->selected + 1, n ? n - 1 : 0);
    else if (key == PS_KEY_UP || key == 'k')
        s->selected = s->selected ? s->selected - 1 : 0;
    else if (key == PS_KEY_PAGEDOWN)
        s->selected = min(s->selected + step, n ? n - 1 : 0);
    else if (key == PS_KEY_PAGEUP)
        s->selected = s->selected > step ? s->selected - step : 0;
    else if (c) {
        if (key == PS_KEY_F2 || ((key == 13 || key == ' ') && c->kind != PS_CONTROL_TOGGLE &&
                                 c->kind != PS_CONTROL_CHOICE)) {
            control_begin(s, c, key == PS_KEY_F2);
            return;
        }
        if (c->kind > PS_CONTROL_CHOICE ||
            (key != PS_KEY_LEFT && key != PS_KEY_RIGHT && key != PS_KEY_HOME && key != PS_KEY_END &&
             key != 13 && key != ' '))
            return;
        if (!control_writable(s, c, &v))
            return;
        if (c->kind == PS_CONTROL_LEVEL) {
            if (key == PS_KEY_HOME)
                v.number = c->minimum;
            else if (key == PS_KEY_END)
                v.number = c->maximum;
            else if (key == PS_KEY_LEFT)
                v.number = v.number - c->minimum < c->step ? c->minimum : v.number - c->step;
            else
                v.number = min(v.number + c->step, c->maximum);
        } else if (c->kind == PS_CONTROL_TOGGLE)
            v.number = key == PS_KEY_HOME || key == PS_KEY_LEFT   ? 0
                       : key == PS_KEY_END || key == PS_KEY_RIGHT ? 1
                                                                  : !v.number;
        else
            v.number = key == PS_KEY_HOME   ? 0
                       : key == PS_KEY_END  ? c->choice_count - 1
                       : key == PS_KEY_LEFT ? (v.number + c->choice_count - 1) % c->choice_count
                                            : (v.number + 1) % c->choice_count;
        control_commit(s, c, &v, v.revision);
    }
}
PS_API int ps_panel_attach(ps_session *s, const ps_panel *panel) {
    unsigned p, i, q, j;
    if (!s || s->editing)
        return 0;
    if (panel) {
        if (!panel->title || !panel->status || !panel->pages || !panel->read || !panel->count ||
            panel->count > 8)
            return 0;
        for (p = 0; p < panel->count; ++p) {
            const ps_control_page *page = &panel->pages[p];
            if (!page->title || page->count > 128 || (page->count && !page->controls))
                return 0;
            for (i = 0; i < page->count; ++i) {
                const ps_control *c = &page->controls[i];
                if (!c->label || !c->group || c->kind > PS_CONTROL_LABEL ||
                    (c->kind == PS_CONTROL_LEVEL &&
                     (c->minimum >= c->maximum || c->maximum > 65535 || !c->step ||
                      c->step > 65535)) ||
                    (c->kind == PS_CONTROL_CHOICE &&
                     (!c->choices || !c->choice_count || c->choice_count > 128)))
                    return 0;
                for (j = 0; c->kind == PS_CONTROL_CHOICE && j < c->choice_count; ++j)
                    if (!c->choices[j])
                        return 0;
                for (q = 0; q <= p; ++q)
                    for (j = 0; j < (q == p ? i : panel->pages[q].count); ++j)
                        if (c->id == panel->pages[q].controls[j].id)
                            return 0;
            }
        }
    }
    if (s->control_view)
        table_close(s);
    s->panel = panel;
    zero(s->control_closed,sizeof s->control_closed);
    s->control_header = 0;
    return 1;
}
PS_API int ps_panel_open(ps_session *s, unsigned page) {
    if (!s || ps_form_active(s) || !s->panel || page >= s->panel->count || !(s->io.flags & PS_INTERACTIVE))
        return 0;
    s->control_view = 1;
    s->dialog = 0;
    s->control_header = 0;
    table_open(s, page);
    s->notice[0] = 0;
    return 1;
}
PS_API void ps_panel_refresh(ps_session *s) {
    if (s)
        s->dirty = 1;
}

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
    static const char *const themes[] = {"Violet", "Red", "Lumen", "One Dark", "Semantic"};
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
        {10, "APPEARANCE", "Color policy", PS_CONTROL_CHOICE, 0, 4, 1, themes, 5},
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
    if (argc == 1 || equal(argv[1], "open")) {
        if (argc > 3 || (argc == 3 && (!control_decimal(argv[2], &id) || !id)))
            return 2;
        if (argc == 3)
            --id;
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
