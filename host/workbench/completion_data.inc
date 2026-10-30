        static const char *const task[] = {"add", "done", "remove", "rename", "list"};
        static const char *const board[] = {"tasks", "variables", "jobs", "session"};
        static const char *const controls[] = {"open", "list", "get", "set", "dashboard"};
        static const char *const theme[] = {"default", "cyan", "violet", "red", "onedark"};
        static const char *const display[] = {"color", "glyphs"}, *const colors[] = {"rgb", "ansi", "mono"}, *const glyphs[] = {"unicode", "ascii"};
        static const char *const txt[] = {"upper", "lower", "trim", "slug"};
        static const char *const hash_options[] = {"--rounds"};
        const char *const *opts = 0;
        unsigned count = 0;
