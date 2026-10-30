    g.columns = s->tab == 0 ? tasks : s->tab == 1 ? pairs : s->tab == 2 ? jobs : stats;
    g.count = (s->tab == 0 || s->tab == 2) ? 3 : 2;
