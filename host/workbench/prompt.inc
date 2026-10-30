        ink(s, 1);
        ps_write(s, s->application_title);
        ink(s, 0);
        out(s, " [");
        ps_number(s, s->task_count);
        out(s, "] ");
        ink(s, 5);
        out(s, "> ");
