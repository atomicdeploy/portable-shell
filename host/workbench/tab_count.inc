    if (s->tab == 0)
        return visible(s, ids);
    if (s->tab == 1)
        return s->var_count;
    if (s->tab == 2)
        return s->job.total ? 1 : 0;
    return 10;
