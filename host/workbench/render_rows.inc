else if (s->tab == 0)
                task_row(s, ids[idx], &grid);
            else if (s->tab == 1)
                pair_row(s, s->vars[idx].key, s->vars[idx].value, &grid);
            else if (s->tab == 3)
                stat_row(s, idx, &grid);
            else {
                ink(s, 4);
                unsigned pct = s->job.total ? s->job.done * 100 / s->job.total : 0;
                grid_text(s, s->job.name, grid.widths[0]);
                grid_divide(s);
                progress_bar(s, grid.widths[1] - 5);
                spaces(s, pct >= 100 ? 0 : pct >= 10 ? 1 : 2);
                grid_divide(s);
                ink(s, s->job.cancelled ? 3 : s->job.done < s->job.total ? 2 : 5);
                field(s,
                      s->job.cancelled             ? "Stopped"
                      : s->job.done < s->job.total ? "Running"
                                                   : "Done",
                      grid.widths[2]);
            }
