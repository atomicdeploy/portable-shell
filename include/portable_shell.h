#ifndef PORTABLE_SHELL_H
#define PORTABLE_SHELL_H
#ifdef __cplusplus
extern "C" {
#endif
/* OS-independent C99. Caller-owned storage, no allocator/global mutable state.
   Default backend is freestanding; libc primitives are an explicit build option. */
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
#define PS_VERSION "0.14.0"
#ifndef PS_WORKSPACE_INLINE_EDIT
#define PS_WORKSPACE_INLINE_EDIT 1
#endif
#ifndef PS_CONTROL_SECTION_ROWS
#define PS_CONTROL_SECTION_ROWS 1
#endif
#if PS_LINE < 128 || PS_HISTORY < 1 || PS_ROWS < 1 || PS_COMMANDS < 20
#error "PortableShell needs PS_LINE >= 128, PS_HISTORY/PS_ROWS >= 1, PS_COMMANDS >= 20"
#endif
typedef unsigned char ps_byte;
typedef char
    ps_requires_8_bit_bytes[((unsigned char)256 == 0 && (unsigned char)255 == 255) ? 1 : -1];
typedef unsigned int ps_u32;
typedef char ps_requires_32_bit_unsigned[(sizeof(ps_u32) == 4) ? 1 : -1];
typedef struct ps_session ps_session;
/* Borrowed immutable definitions; retain them and context until detach/session end.
   Callbacks run on the owning loop, must not reenter the session. No allocation.
   read supplies observed state, never a speculative requested value. write must
   atomically check expected_revision and return 0 rejected, 1 applied, 2 pending.
   A pending provider exposes PS_CONTROL_PENDING until readback or failure. */
enum {
    PS_CONTROL_TOGGLE,
    PS_CONTROL_LEVEL,
    PS_CONTROL_CHOICE,
    PS_CONTROL_TEXT,
    PS_CONTROL_ACTION,
    PS_CONTROL_LABEL
};
enum { PS_CONTROL_VALID = 1, PS_CONTROL_PENDING = 2, PS_CONTROL_LOCKED = 4 };
typedef struct {
    unsigned id;
    const char *group, *label;
    unsigned kind, minimum, maximum, step; /* numeric range <= 65535 */
    const char *const *choices;
    unsigned choice_count;
} ps_control;
typedef struct {
    const char *title;
    const ps_control *controls;
    unsigned count;
} ps_control_page;
typedef struct {
    unsigned number, flags;
    ps_u32 revision;
    char text[64];
} ps_control_value;
typedef int (*ps_control_read_fn)(void *, unsigned id, ps_control_value *observed);
typedef int (*ps_control_write_fn)(void *, unsigned id, const ps_control_value *requested,
                                   ps_u32 expected_revision);
typedef struct {
    const char *title, *status;
    const ps_control_page *pages;
    unsigned count; /* 1..8 pages, <=128 rows each, unique stable IDs */
    ps_control_read_fn read;
    ps_control_write_fn write;
    void *context;
} ps_panel;
PS_API int ps_panel_attach(ps_session *, const ps_panel *); /* NULL detaches */
PS_API int ps_panel_open(ps_session *, unsigned page);
PS_API void ps_panel_refresh(ps_session *); /* after provider events, redraw on tick */
typedef struct {
    const char *title;
    unsigned minimum, maximum, weight; /* maximum=0 means flexible; weight 0..16 */
} ps_column;
enum { PS_TABLE_GRID = 1, PS_TABLE_RED_BORDER = 2, PS_TABLE_TITLE_BAR = 4,
       PS_TABLE_CATEGORY_COLUMN = 8, PS_TABLE_COMPACT_SECTIONS = 16,
       PS_TABLE_TABS_INSIDE = 32, PS_TABLE_DEFAULT = 7 };
/* Fits 1..8 columns into a framed width (max 240), including padding/separators.
   Returns occupied width, or 0 if minima do not fit / definitions are invalid. */
PS_API unsigned ps_layout_columns(unsigned width, const ps_column *columns, unsigned count,
                                  unsigned *widths);
PS_API void ps_set_table_style(ps_session *, unsigned flags);
typedef void (*ps_write_fn)(void *context, const ps_byte *bytes, unsigned count);
typedef int (*ps_command_fn)(ps_session *, int argc, const char *const *argv, void *context);
/* Raw tail starts immediately after the first space/tab; quotes and all remaining
   whitespace are preserved. Only command-name matching is case folded. */
typedef int (*ps_raw_command_fn)(ps_session *, const char *tail, void *context);
/* Completion providers are queried by Tab and interactive ghost previews. Return
   stable borrowed strings; enumeration must not mutate device/application state.
   PS_NO_SUGGESTIONS disables preview queries while retaining explicit Tab. */
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
enum {
    PS_INTERACTIVE = 1,
    PS_COLOR = 2,
    PS_UNICODE = 4,
    PS_TELNET = 8,
    PS_TRUECOLOR = 16,
    PS_CASE_INSENSITIVE = 32,
    PS_NO_BUILTINS = 64,
    PS_LEGACY_KEYS = 128,
    PS_NO_BELL = 256,
    PS_NO_SUGGESTIONS = 512
};
enum { PS_STYLE_DEFAULT, PS_STYLE_PROMPT, PS_STYLE_SUCCESS, PS_STYLE_WARNING, PS_STYLE_ERROR,
       PS_STYLE_GREEN };
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
PS_API const char *ps_memory_backend(void); /* "portable" or "libc"; no allocation */
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
PS_API int ps_unregister(ps_session *s, const char *name);
/* Metadata remains borrowed until the next registry mutation. */
PS_API unsigned ps_command_count(const ps_session *s);
PS_API const ps_command *ps_command_at(const ps_session *s, unsigned index);
PS_API int ps_register_raw(ps_session *s, const ps_command *metadata, ps_raw_command_fn run);
/* Text is copied, max 63 UTF-8 bytes, no control characters. NULL restores default. */
PS_API int ps_set_prompt(ps_session *s, const char *text, unsigned style);
PS_API void ps_set_uppercase_echo(ps_session *s, int enabled);
/* Safe external event output; preserves pending input/cursor. In the board it
   becomes the contextual notice. Call from the owning loop, not from write(). */
PS_API void ps_message(ps_session *s, unsigned style, const char *text);
PS_API void ps_clear(ps_session *s);
PS_API void ps_request_exit(ps_session *s);
PS_API int ps_bind(ps_session *s, unsigned key, ps_key_fn callback, void *context);
PS_API void ps_key(ps_session *s, unsigned key);
PS_API void ps_write(ps_session *s, const char *text); /* safe printable output, strips controls */
PS_API void ps_newline(ps_session *s);
/* Emits byte 7 through the normal transport in interactive sessions unless muted.
   The receiving terminal decides whether to sound or visually indicate the bell. */
PS_API void ps_ring_bell(ps_session *s);
enum { PS_HELP_HEADINGS, PS_HELP_COMPACT };
/* NULL name lists the registry; non-NULL prints Usage. Returns entries printed. */
PS_API int ps_print_help(ps_session *s, const char *name, unsigned layout);
PS_API void ps_number(ps_session *s, ps_u32 value);
PS_API void ps_progress(ps_session *s, const char *name, unsigned done, unsigned total);
PS_API void ps_history_clear(ps_session *s);
typedef int (*ps_history_filter_fn)(void *context, const char *line);
/* Additional application admission; default conservative credential filtering
 * always applies. Changing policy clears retained entries. NULL removes hook. */
PS_API void ps_history_filter(ps_session *s, ps_history_filter_fn filter, void *context);
PS_API void ps_destroy_session(ps_session *s); /* close + scrub; storage remains caller-owned */
PS_API unsigned ps_history_count(const ps_session *s);
PS_API const char *ps_history_get(const ps_session *s, unsigned oldest_index);
PS_API int ps_history_add(ps_session *s, const char *line);
/* Retains newest entries. 0 disables history; values above PS_HISTORY clamp. */
PS_API unsigned ps_history_limit(ps_session *s, unsigned limit);
#ifdef __cplusplus
}
#endif
#ifdef PS_ENABLE_LEGACY_API
#include "portable_shell_legacy.h"
#endif
#endif
