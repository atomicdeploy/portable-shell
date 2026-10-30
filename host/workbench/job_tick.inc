    if (s->job.active) {
        while (s->job.done < s->job.total && rounds++ < 128) {
            for (i = 0; i < s->job_length; ++i) {
                s->job.hash ^= (ps_byte)s->job_data[i];
                s->job.hash *= 16777619u;
            }
            ++s->job.done;
        }
        s->dirty = 1;
        if (s->job.done == s->job.total) {
            s->job.active = 0;
            notice(s, "Hash complete");
            if (!s->board) {
                if (s->io.flags & PS_INTERACTIVE)
                    ansi(s, "\r\033[2K");
                job_report(s);
                prompt(s);
            }
        }
    }
