static int builtin(ps_session *s, int argc, const char *const *a, void *ctx) {
    unsigned i, j;
    const char *cmd = a[0];
    (void)ctx;
#if PS_OSC
    if(equal(cmd,"title")){
        char title[PS_LINE];unsigned used=0,k;
        for(i=1;i<(unsigned)argc;++i){
            if(!ps_osc_text_valid(a[i]))return fail(s,"invalid title");
            if(i>1){if(used+1>=sizeof title)return fail(s,"title too long");title[used++]=' ';}
            for(k=0;a[i][k];++k){if(used+1>=sizeof title)return fail(s,"title too long");title[used++]=a[i][k];}
        }
        title[used]=0;ps_set_title(s,title);return 0;
    }
    if(equal(cmd,"osc")){
        if(argc==4&&equal(a[1],"path")){
            if(!ps_terminal_directory(osc_discard,0,a[2],a[3]))return fail(s,"invalid directory");
            ps_report_directory(s,a[2],a[3]);return 0;
        }
        if((argc==3||argc==4)&&equal(a[1],"progress")){
            const char *states[]={"clear","normal","error","busy","paused"};int percent=0;
            for(i=0;i<5;++i)if(equal(a[2],states[i]))break;
            if(i==5||(argc==4&&(!integer(a[3],&percent)||percent<0||percent>100)))return fail(s,"invalid progress");
            ps_terminal_report_progress(s,i,(unsigned)percent);return 0;
        }
        return fail(s,"osc progress clear|normal|error|busy|paused [0..100] | path HOST /PATH");
    }
#endif
    if(equal(cmd,"edit")){
        if(argc>2||(argc==2&&!equal(a[1],"--framed")))return fail(s,"edit [--framed]");
        if(argc==2)ps_editor_set_style(s,PS_EDITOR_HEADER|PS_EDITOR_BORDER);
        return ps_editor_open(s,"Session notes (memory)",s->scratch,document_save_scratch,s)?0:fail(s,"editor unavailable (interactive terminal required)");
    }
    if(equal(cmd,"files")){
        if(argc!=1)return fail(s,"files");
        return ps_file_picker_open(s,"Session files (memory)",session_files,2,session_file_pick,0)?0:fail(s,"file picker requires interactive terminal");
    }
    if(equal(cmd,"menu")){
        int result=0;unsigned first=1,style=PS_MENU_TABLE|PS_MENU_NUMBERS;
        for(;first<(unsigned)argc;++first){if(equal(a[first],"--list")&&PS_MENU_LIST_VIEW)style=PS_MENU_LIST;else if(equal(a[first],"--plain"))style=0;else if(equal(a[first],"--no-numbers"))style&=~PS_MENU_NUMBERS;else if(equal(a[first],"--details"))style|=PS_MENU_DETAILS;else break;}
        ps_menu_set_style(s,style);ps_menu_open(s,&default_menu);
        for(i=first;i<(unsigned)argc;++i){
            int n;const ps_menu *m=s->menu_depth?s->menu_stack[s->menu_depth-1]:0;
            if(!m||!integer(a[i],&n)||n<0||(unsigned)n>m->count||
               (i+1<(unsigned)argc&&n&&!m->items[n-1].submenu)){
                ps_menu_close(s);return fail(s,"menu [NUMBER ...]");
            }
            result=ps_menu_choose(s,n);
            if(result){ps_menu_close(s);return result;}
        }
        if(!(s->io.flags&PS_INTERACTIVE))ps_menu_close(s);
        return result;
    }
    if (equal(cmd, "help")) {
        const char *name = 0;
        unsigned layout = PS_HELP_HEADINGS;
        for (i = 1; i < (unsigned)argc; ++i) {
            if (equal(a[i], "--compact")) layout = PS_HELP_COMPACT;
            else if (equal(a[i], "--headings")) layout = PS_HELP_HEADINGS;
            else if (!name) name = a[i];
            else return fail(s, "help [--headings|--compact] [command]");
        }
        return ps_print_help(s, name, layout) ? 0 : fail(s, "Unknown command");
    }
    if (equal(cmd, "bell")) {
        ps_ring_bell(s);
        return 0;
    }
    if (equal(cmd, "echo")) {
        for (i = 1; i < (unsigned)argc; ++i) {
            if (i > 1)
                out(s, " ");
            ps_write(s, a[i]);
        }
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "args")) {
        for (i = 1; i < (unsigned)argc; ++i) {
            ps_number(s, i - 1);
            out(s, " [");
            ps_number(s, length(a[i]));
            out(s, "] ");
            ps_write(s, a[i]);
            ps_newline(s);
        }
        return 0;
    }
    if (equal(cmd, "controls")) {
        int result;
        s->notice[0] = 0;
        result = controls_command(s, argc, a);
        if (result && !s->board)
            return fail(s, s->notice[0] ? s->notice
                                        : "controls [open [page]|dashboard|list|get ID|set ID VALUE]");
        return result;
    }
    if (equal(cmd, "board")) {
        unsigned tab = 0;
        if (argc > 2)
            return fail(s, "board [tasks|variables|jobs|session]");
        if (argc == 2) {
            if (equal(a[1], "variables"))
                tab = 1;
            else if (equal(a[1], "jobs"))
                tab = 2;
            else if (equal(a[1], "session"))
                tab = 3;
            else if (!equal(a[1], "tasks"))
                return fail(s, "unknown tab");
        }
        if (!(s->io.flags & PS_INTERACTIVE))
            return fail(s, "board requires interactive mode");
        s->control_view = s->dialog = 0;
        table_open(s, tab);
        return 0;
    }
    if (equal(cmd, "task")) {
        if (argc == 1 || (argc == 2 && equal(a[1], "list"))) {
            for (i = 0; i < s->task_count; ++i) {
                ps_number(s, s->tasks[i].id);
                out(s, s->tasks[i].done ? " [x] " : " [ ] ");
                ps_write(s, s->tasks[i].title);
                ps_newline(s);
            }
            return 0;
        }
        if (argc == 3 && equal(a[1], "add")) {
            if (!add_task(s, a[2]))
                return fail(s, "task must be 1..127 bytes; table capacity 32");
            ps_number(s, s->tasks[s->task_count - 1].id);
            ps_newline(s);
            return 0;
        }
        if (argc >= 3) {
            int id;
            if (!integer(a[2], &id) || id < 1)
                return fail(s, "invalid task id");
            for (i = 0; i < s->task_count && s->tasks[i].id != (unsigned)id; ++i) {
            }
            if (i == s->task_count)
                return fail(s, "task not found");
            if (argc == 3 && equal(a[1], "done")) {
                s->tasks[i].done ^= 1;
                return 0;
            }
            if (argc == 3 && equal(a[1], "remove")) {
                move(s->tasks + i, s->tasks + i + 1, (s->task_count - i - 1) * sizeof(Task));
                --s->task_count;
                return 0;
            }
            if (argc == 4 && equal(a[1], "rename")) {
                if (!*a[3] || length(a[3]) >= PS_VALUE)
                    return fail(s, "invalid title length");
                str(s->tasks[i].title, a[3], PS_VALUE);
                return 0;
            }
        }
        return fail(s, "task add TITLE | done ID | remove ID | rename ID TITLE | list");
    }
    if (equal(cmd, "set") || equal(cmd, "get") || equal(cmd, "unset")) {
        int set = equal(cmd, "set"), del = equal(cmd, "unset");
        if ((set && argc != 3) || (del && argc != 2) || (!set && !del && argc > 2))
            return fail(s, "set KEY VALUE | get [KEY] | unset KEY");
        if (argc > 1 && equal(a[1], "uppercase_echo")) {
            int result;
            if (del) return fail(s, "Use set uppercase_echo false");
            result = session_uppercase_setting(s, set ? a[2] : 0);
            if (!result && set) ++s->local_revision;
            return result;
        }
        if (argc == 1) {
            ps_write(s, "uppercase_echo=");
            session_uppercase_setting(s, 0);
            for (i = 0; i < s->var_count; ++i) {
                ps_write(s, s->vars[i].key);
                out(s, "=");
                ps_write(s, s->vars[i].value);
                ps_newline(s);
            }
            return 0;
        }
        for (i = 0; i < s->var_count && !equal(s->vars[i].key, a[1]); ++i) {
        }
        if (set) {
            if (!*a[1] || length(a[1]) >= 32 || length(a[2]) >= PS_VALUE)
                return fail(s, "key/value too long or empty key");
            for (j = 0; a[1][j]; ++j)
                if (!((a[1][j] >= 'a' && a[1][j] <= 'z') || (a[1][j] >= 'A' && a[1][j] <= 'Z') ||
                      (a[1][j] >= '0' && a[1][j] <= '9') || a[1][j] == '_' || a[1][j] == '-' ||
                      a[1][j] == '.'))
                    return fail(s, "key requires letters, digits, _, - or .");
            if (i == PS_ROWS)
                return fail(s, "variable capacity reached");
            if (i == s->var_count)
                ++s->var_count;
            str(s->vars[i].key, a[1], 32);
            str(s->vars[i].value, a[2], PS_VALUE);
            return 0;
        }
        if (i == s->var_count)
            return fail(s, "key not found");
        if (del) {
            move(s->vars + i, s->vars + i + 1, (s->var_count - i - 1) * sizeof(Variable));
            --s->var_count;
        } else {
            ps_write(s, s->vars[i].value);
            ps_newline(s);
        }
        return 0;
    }
    if (equal(cmd, "text")) {
        char b[PS_LINE];
        unsigned n = 0, start = 0, end, sep = 0;
        if (argc != 3)
            return fail(s, "text upper|lower|trim|slug TEXT");
        end = length(a[2]);
        if (equal(a[1], "trim")) {
            while (a[2][start] == ' ' || a[2][start] == '\t' || a[2][start] == '\n')
                ++start;
            while (end > start &&
                   (a[2][end - 1] == ' ' || a[2][end - 1] == '\t' || a[2][end - 1] == '\n'))
                --end;
        } else if (!equal(a[1], "upper") && !equal(a[1], "lower") && !equal(a[1], "slug"))
            return fail(s, "unknown transform");
        for (i = start; i < end && n + 1 < PS_LINE; ++i) {
            unsigned char c = (unsigned char)a[2][i];
            if (equal(a[1], "upper") && c >= 'a' && c <= 'z')
                c -= 32;
            else if ((equal(a[1], "lower") || equal(a[1], "slug")) && c >= 'A' && c <= 'Z')
                c += 32;
            if (equal(a[1], "slug")) {
                if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
                    sep = n ? 1 : 0;
                    continue;
                }
                if (sep && n + 2 < PS_LINE)
                    b[n++] = '-';
                sep = 0;
            }
            b[n++] = (char)c;
        }
        b[n] = 0;
        ps_write(s, b);
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "calc")) {
        int x, y, r = 0;
        unsigned ax, ay, lim;
        char op;
        if (argc != 4 || !integer(a[1], &x) || !integer(a[3], &y) || length(a[2]) != 1)
            return fail(s, "calc INTEGER +|-|*|/|% INTEGER");
        op = *a[2];
        if (op == '+') {
            if ((y > 0 && x > 2147483647 - y) || (y < 0 && x < (-2147483647 - 1) - y))
                return fail(s, "integer overflow");
            r = x + y;
        } else if (op == '-') {
            if ((y < 0 && x > 2147483647 + y) || (y > 0 && x < (-2147483647 - 1) + y))
                return fail(s, "integer overflow");
            r = x - y;
        } else if (op == '*') {
            ax = x < 0 ? 0u - (unsigned)x : (unsigned)x;
            ay = y < 0 ? 0u - (unsigned)y : (unsigned)y;
            lim = ((x < 0) != (y < 0)) ? 2147483648u : 2147483647u;
            if (ay && ax > lim / ay)
                return fail(s, "integer overflow");
            ax *= ay;
            r = ((x < 0) != (y < 0)) ? (ax == 2147483648u ? (-2147483647 - 1) : -(int)ax) : (int)ax;
        } else if (op == '/' || op == '%') {
            if (!y)
                return fail(s, "division by zero");
            if (x == (-2147483647 - 1) && y == -1)
                return fail(s, "integer overflow");
            r = op == '/' ? x / y : x % y;
        } else
            return fail(s, "unknown operator");
        signed_number(s, r);
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "hash")) {
        const char *rounds;
        int n = 1;
        if (!(argc == 2 || (argc == 3 && prefix(a[2], "--rounds=")) ||
              (argc == 4 && equal(a[2], "--rounds"))))
            return fail(s, "hash TEXT [--rounds N]");
        rounds = ps_option(argc, a, "rounds");
        if (rounds && (!integer(rounds, &n) || n < 1 || n > 1000000))
            return fail(s, "rounds must be 1..1000000");
        if (s->job.active)
            return fail(s, "job already running");
        str(s->job_data, a[1], PS_LINE);
        s->job_length = length(s->job_data);
        str(s->job.name, "FNV-1a stream", 40);
        s->job.total = (unsigned)n;
        s->job.done = 0;
        s->job.active = 1;
        s->job.cancelled = 0;
        s->job.hash = 2166136261u;
        notice(s, "Hash job started");
        return 0;
    }
    if (equal(cmd, "jobs")) {
        if (argc == 2 && equal(a[1], "cancel")) {
            s->job.active = 0;
            s->job.cancelled = 1;
            notice(s, "Job cancelled");
        } else if (argc != 1)
            return fail(s, "jobs [cancel]");
        if (s->job.total)
            job_report(s);
        return 0;
    }
    if (equal(cmd, "history")) {
        if (argc == 2 && equal(a[1], "clear")) {
            ps_history_clear(s);
            return 0;
        }
        if (argc != 1)
            return fail(s, "history [clear]");
        for (i = 0; i < s->history_count; ++i) {
            ps_number(s, i + 1);
            out(s, "  ");
            ps_write(s, ps_history_get(s, i));
            ps_newline(s);
        }
        return 0;
    }
    if (equal(cmd, "bind")) {
        unsigned key;
        if (argc == 1) {
            for (i = 0; i < s->binding_count; ++i) {
                ps_number(s, s->bindings[i].key);
                out(s, "  ");
                ps_write(s, s->bindings[i].command);
                ps_newline(s);
            }
            return 0;
        }
        if (argc != 3 || a[1][0] != 'F' || a[1][1] < '1' || a[1][1] > '4' || a[1][2] ||
            length(a[2]) >= 64)
            return fail(s, "bind F1|F2|F3|F4 COMMAND");
        key = PS_KEY_F1 + (unsigned)(a[1][1] - '1');
        if (ps_bind(s, key, 0, 0))
            return fail(s, "binding capacity reached");
        for (i = 0; i < s->binding_count; ++i)
            if (s->bindings[i].key == key)
                str(s->bindings[i].command, a[2], 64);
        return 0;
    }
    if(equal(cmd,"segments")){
        unsigned short digits[PS_SEG_MAX_CELLS];int count;
        ps_segment_style display=ps_segment_default_style();
        if(argc<2)return fail(s,"segments TEXT [--large] [--red] [--mono] [--ascii]");
        display.unicode=!!(s->io.flags&PS_UNICODE);
        display.color=(s->io.flags&(PS_INTERACTIVE|PS_COLOR))==(PS_INTERACTIVE|PS_COLOR);
        for(i=2;i<(unsigned)argc;++i){
            if(equal(a[i],"--large")){display.horizontal=4;display.vertical=3;}
            else if(equal(a[i],"--red"))display.foreground=203;
            else if(equal(a[i],"--mono"))display.color=0;
            else if(equal(a[i],"--ascii"))display.unicode=0;
            else return fail(s,"unknown display option");
        }
        count=ps_segment_parse(a[1],digits,PS_SEG_MAX_CELLS);
        if(count<=0)return fail(s,"unsupported display text or too many cells");
        if(!ps_segment_width(digits,(unsigned)count,&display)||ps_segment_width(digits,(unsigned)count,&display)>=s->io.columns)
            return fail(s,"display does not fit terminal");
        frame_begin(s);
        ps_segment_render(digits,(unsigned)count,&display,s->io.columns-1,segment_sink,s);
        frame_end(s);s->prompt_rows_drawn=0;return 0;
    }
    if(equal(cmd,"prompt")){
        static const char *const names[]={"default","heading","success","warning","error","green","muted","command","blue"};
        ps_prompt_part parts[16];unsigned n=0,j;
        if(argc==2&&equal(a[1],"reset"))return ps_set_prompt_parts(s,0,0);
        if(argc<3 || !(argc&1))return fail(s,"prompt reset | STYLE TEXT [STYLE TEXT ...]");
        for(i=1;i<(unsigned)argc;i+=2){
            for(j=0;j<9&&!equal(a[i],names[j]);++j){}
            if(j==9||n==16)return fail(s,"unknown prompt style");
            parts[n].text=a[i+1];parts[n++].style=j;
        }
        return ps_set_prompt_parts(s,parts,n)?fail(s,"prompt exceeds limits or contains controls"):0;
    }
    if (equal(cmd, "theme") || equal(cmd, "display")) {
        const char *choice;
        if(equal(cmd,"display")){
            if(argc!=3)return fail(s,"display color rgb|ansi|mono; display glyphs unicode|ascii");
            if(equal(a[1],"color")&&(equal(a[2],"rgb")||equal(a[2],"ansi")||equal(a[2],"mono")))
                choice=equal(a[2],"rgb")?"color":a[2];
            else if(equal(a[1],"glyphs")&&(equal(a[2],"unicode")||equal(a[2],"ascii")))choice=a[2];
            else return fail(s,"unknown display setting");
        }else{
            if(argc!=2)return fail(s,"theme default|cyan|violet|red|onedark");
            choice=equal(a[1],"semantic")?"default":equal(a[1],"lumen")?"cyan":a[1];
        }
#ifdef PS_ASCII_ONLY
        if (equal(choice, "color") || equal(choice, "unicode") || equal(choice, "ansi"))
            return fail(s, "disabled in ASCII-only build");
#endif
        if (equal(choice, "violet") || equal(choice, "red") || equal(choice, "cyan") ||
            equal(choice, "onedark") || equal(choice, "default")) {
            s->theme = equal(choice, "default") ? PS_THEME_DEFAULT : equal(choice, "onedark") ? 3
                       : equal(choice, "cyan") ? 2
                       : equal(choice, "red")   ? 1
                                              : 0;
        } else if (equal(choice, "color"))
            s->io.flags |= PS_COLOR | PS_TRUECOLOR;
        else if (equal(choice, "ansi")) {
            s->io.flags |= PS_COLOR;
            s->io.flags &= ~PS_TRUECOLOR;
        } else if (equal(choice, "mono")) {
            s->io.flags &= ~PS_COLOR;
            reset(s);
        } else if (equal(choice, "ascii"))
            s->io.flags &= ~PS_UNICODE;
        else if (equal(choice, "unicode"))
            s->io.flags |= PS_UNICODE;
        else
            return fail(s, "unknown theme");
        if (s->board) {
            reset(s);
            s->canvas_kind=0;
        }
        s->dirty = 1;
        return 0;
    }
    if (equal(cmd, "clear")) {
        s->prompt_rows_drawn=0;ansi(s, "\033[2J\033[H");
        return 0;
    }
    if (equal(cmd, "status")) {
#if PS_REPORT
        if(argc==2&&equal(a[1],"--report")){
            char storage[11],executions[11],tasks[11],received[11],sent[11];
            ps_report_row rows[]={
                {"Session",0,PS_VERSION,PS_REPORT_SECTION},
                {"Storage","caller-owned bytes",storage,PS_REPORT_NUMBER},
                {"Heap","core allocations","0",PS_REPORT_NUMBER},
                {"Backend",0,ps_memory_backend(),0},
                {"Activity",0,0,PS_REPORT_SECTION|PS_REPORT_GAP},
                {"Commands","executed",executions,PS_REPORT_NUMBER},
                {"Tasks",0,tasks,PS_REPORT_NUMBER},
                {"Input","bytes",received,PS_REPORT_NUMBER},
                {"Output","bytes before this report",sent,PS_REPORT_NUMBER}
            };
            dec(storage,sizeof *s);dec(executions,s->executions);dec(tasks,s->task_count);
            dec(received,s->bytes_in);dec(sent,s->bytes_out);
            ps_print_report(s,rows,sizeof rows/sizeof rows[0],PS_REPORT_DOTS);
            return 0;
        }
#endif
        if(argc!=1)return fail(s,"status [--report]");
        out(s, "version=" PS_VERSION " session_bytes=");
        ps_number(s, sizeof *s);
        out(s, " heap_bytes=0 memory_backend=");
        out(s, ps_memory_backend());
        out(s, " commands=");
        ps_number(s, s->executions);
        out(s, " tasks=");
        ps_number(s, s->task_count);
        out(s, " input_bytes=");
        ps_number(s, s->bytes_in);
        out(s, " output_bytes=");
        ps_number(s, s->bytes_out);
        ps_newline(s);
        return 0;
    }
    if (equal(cmd, "exit")) {
        s->exit = 1;
        ps_close(s);
        return 0;
    }
    return fail(s, "unknown command");
}
