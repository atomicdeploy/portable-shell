static unsigned visible(ps_session *s, unsigned *indices) {
    unsigned i, n = 0;
    for (i = 0; i < s->task_count; ++i)
        if (contains(s->tasks[i].title, s->filter))
            indices[n++] = i;
    return n;
}
