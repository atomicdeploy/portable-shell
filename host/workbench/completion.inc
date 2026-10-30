            if (!match_count) {
                if (equal(args.argv[0], "task") && args.argc == 1) {
                    opts = task;
                    count = 5;
                } else if (equal(args.argv[0], "board")) {
                    opts = board;
                    count = 4;
                } else if (equal(args.argv[0], "controls") && args.argc == 1) {
                    opts = controls;
                    count = PS_DASHBOARD ? 5 : 4;
                } else if (equal(args.argv[0], "theme")) {
                    opts = theme;
                    count = 5;
                } else if (equal(args.argv[0], "display")) {
                    if(args.argc==1){opts=display;count=2;}
                    else if(equal(args.argv[1],"color")){opts=colors;count=3;}
                    else if(equal(args.argv[1],"glyphs")){opts=glyphs;count=2;}
                } else if (equal(args.argv[0], "text") && args.argc == 1) {
                    opts = txt;
                    count = 4;
                } else if (equal(args.argv[0], "hash") && args.argc >= 2) {
                    opts = hash_options;
                    count = 1;
                } else if (equal(args.argv[0], "get") || equal(args.argv[0], "set") ||
                           equal(args.argv[0], "unset")) {
                    for (i = 0; i < s->var_count && match_count < PS_COMMANDS; ++i)
                        if (prefix(s->vars[i].key, token))
                            matches[match_count++] = s->vars[i].key;
                } else if (equal(args.argv[0], "help")) {
                    for (i = 0; i < s->command_count; ++i)
                        if (name_match(s, s->commands[i].name, token, 1))
                            matches[match_count++] = s->commands[i].name;
                }
                for (i = 0; i < count; ++i)
                    if (prefix(opts[i], token))
                        matches[match_count++] = opts[i];
            }
