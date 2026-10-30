static void hex(ps_session *s, ps_u32 n) {
    char b[9];
    unsigned i;
    for (i = 0; i < 8; ++i)
        b[i] = "0123456789abcdef"[(n >> (28 - i * 4)) & 15];
    b[8] = 0;
    out(s, b);
}
static void job_report(ps_session *s) {
    ps_write(s, s->job.name);
    out(s, "  ");
    ps_number(s, s->job.done);
    out(s, "/");
    ps_number(s, s->job.total);
    out(s, s->job.cancelled ? " cancelled" : s->job.done < s->job.total ? " running" : " complete");
    out(s, "  fnv1a=");
    hex(s, s->job.hash);
    ps_newline(s);
}
