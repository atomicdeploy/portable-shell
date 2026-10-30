          s->control_view ? control_hint(s)
          : s->editing    ? "Enter save   Esc cancel"
          : s->tab == 0   ? (w < 60 ? "n new  Enter edit  d done  / filter"
                                    : "n new   Enter edit   d done   Space select   / filter")
          : s->tab == 1   ? "Enter edit value   Del remove"
          : s->tab == 2   ? "c cancel job"
                          : "Live session counters",
