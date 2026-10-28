#ifndef PORTABLE_SHELL_H
#define PORTABLE_SHELL_H
#ifdef __cplusplus
extern "C" {
#endif
/* Freestanding C99. No system headers, runtime, allocator, or global state. */
#if defined(PORTABLE_SHELL_DLL) && defined(_WIN32)
#define PS_API __declspec(dllexport)
#else
#define PS_API
#endif
#ifndef PS_LINE
#define PS_LINE 512
#endif
#ifndef PS_HISTORY
#define PS_HISTORY 24
#endif
#ifndef PS_ROWS
#define PS_ROWS 32
#endif
#ifndef PS_COMMANDS
#define PS_COMMANDS 40
#endif
#define PS_ARGS 32
#define PS_VALUE 128
#define PS_VERSION "0.1.0"
#if PS_LINE < 128 || PS_HISTORY < 1 || PS_ROWS < 1 || PS_COMMANDS < 18
#error "PortableShell needs PS_LINE >= 128, PS_HISTORY/PS_ROWS >= 1, PS_COMMANDS >= 18"
#endif
typedef unsigned char ps_byte;
typedef unsigned int ps_u32;
typedef char ps_requires_32_bit_unsigned[(sizeof(ps_u32) == 4) ? 1 : -1];
typedef struct ps_session ps_session;
typedef void (*ps_write_fn)(void *context, const ps_byte *bytes, unsigned count);
typedef int (*ps_command_fn)(ps_session *, int argc, const char *const *argv, void *context);
typedef unsigned (*ps_complete_fn)(ps_session *, int argc, const char *const *argv,
                                   const char *prefix, const char **items, unsigned capacity,
                                   void *context);
typedef void (*ps_key_fn)(ps_session *, unsigned key, void *context);
typedef struct {
    const char *name, *usage, *category;
    ps_command_fn run;
    ps_complete_fn complete;
    void *context;
} ps_command;
typedef struct {
    int argc;
    char *argv[PS_ARGS];
    char storage[PS_LINE];
    unsigned error_at;
    const char *error;
} ps_args;
typedef struct {
    ps_write_fn write;
    void *context;
    unsigned columns, rows;
    unsigned flags;
} ps_config;
enum { PS_INTERACTIVE = 1, PS_COLOR = 2, PS_UNICODE = 4, PS_TELNET = 8 };
enum {
    PS_KEY_UP = 256,
    PS_KEY_DOWN,
    PS_KEY_LEFT,
    PS_KEY_RIGHT,
    PS_KEY_HOME,
    PS_KEY_END,
    PS_KEY_DELETE,
    PS_KEY_PAGEUP,
    PS_KEY_PAGEDOWN,
    PS_KEY_BACKTAB,
    PS_KEY_F1,
    PS_KEY_F2,
    PS_KEY_F3,
    PS_KEY_F4,
    PS_KEY_WORDLEFT,
    PS_KEY_WORDRIGHT
};
PS_API unsigned ps_sizeof_session(void);
PS_API unsigned ps_alignof_session(void);
/* Memory must be aligned (normal static storage / malloc alignment is sufficient).
   Callback consumes ALL bytes synchronously, must not reenter this session.
   One owner/event loop per session; separate sessions share no mutable state. */
PS_API ps_session *ps_init(void *memory, unsigned bytes, const ps_config *config);
PS_API void ps_start(ps_session *s);
PS_API void ps_feed(ps_session *s, const void *bytes, unsigned count);
PS_API void ps_tick(ps_session *s, ps_u32 monotonic_ms);
PS_API void ps_resize(ps_session *s, unsigned columns, unsigned rows);
PS_API void ps_finish(ps_session *s); /* EOF: execute final unterminated batch line. */
PS_API void ps_close(ps_session *s);  /* restore terminal modes */
PS_API int ps_exited(const ps_session *s);
PS_API int ps_status(const ps_session *s);
PS_API int ps_execute(ps_session *s, const char *line);
PS_API int ps_execute_argv(ps_session *s, int argc, const char *const *argv);
PS_API int ps_parse(const char *line, ps_args *out);
PS_API const char *ps_option(int argc, const char *const *argv, const char *name);
PS_API int ps_register(ps_session *s, const ps_command *command);
PS_API int ps_bind(ps_session *s, unsigned key, ps_key_fn callback, void *context);
PS_API void ps_key(ps_session *s, unsigned key);
PS_API void ps_write(ps_session *s, const char *text); /* safe printable output, strips controls */
PS_API void ps_newline(ps_session *s);
PS_API void ps_number(ps_session *s, ps_u32 value);
PS_API void ps_progress(ps_session *s, const char *name, unsigned done, unsigned total);
PS_API unsigned ps_history_count(const ps_session *s);
PS_API const char *ps_history_get(const ps_session *s, unsigned oldest_index);
PS_API int ps_history_add(ps_session *s, const char *line);
#ifdef __cplusplus
}
#endif
#endif
