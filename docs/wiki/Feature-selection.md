# Consumer-owned feature selection

Do not select features by a donor or device product name. Select an API composition,
then explicitly define capacities and features for every translation unit that shares
its objects. Compile the full library with the same definitions as its consumers.

## Choose the composition

| Requirement | Include/link | What is not pulled in |
| --- | --- | --- |
| Existing firmware owns commands; needs editing only | `portable_editor.h` or `portable_shell_editor.hpp` | Full session, command catalogue, host, TUI |
| Existing firmware owns values; needs a view | `portable_device_view.hpp` | OS adapters, hardware actions, full-session command catalogue |
| Application needs registered commands and full UI | Link `portable_shell`, include `portable_shell.h` | Workbench commands, OS execution, device compatibility catalogue |
| Desktop demonstration | `portable_shell_workbench` and native host | This composition intentionally includes demo commands |

The optional device command catalogue is an existing compatibility consumer, not
an architectural dependency or the recommended integration API. New consumers use
`ps_register`, `ps_register_raw`, `ps_register_definition`, and typed `ps_settings`.
Do not link it just to obtain Telnet, colors, prompts, parsing or number formatting.
Application validation, read-only settings, storage and hardware remain callbacks.

## Small event editor, independently chosen options

See [a consumer-owned configuration](https://github.com/atomicdeploy/portable-shell/blob/audit/consumer-feature-selection/examples/editor_config.h). Include it
before the editor header in every relevant source. It explicitly chooses a 32-byte
line, zero history and zero undo. No architecture test removes functionality.
`PS_EDITOR_HISTORY=0` removes the history and saved-history-draft arrays; Up/Down
ring the editor bell instead of reading absent history. Completion from application
candidates remains usable. `PS_EDITOR_UNDO_DEPTH=0` removes undo snapshots.
Suggestion capacity remains separately bounded; a two-byte buffer admits only a
one-byte suffix. Not calling suggestion/completion functions avoids those paths in
optimized header builds. This is not a claim that every field has an exclusion flag.

`ps_editor` is byte-oriented; the full session provides richer Unicode cursor/UI
behavior. They remain distinct higher-level editors, sharing primitives and undo.
Do not instantiate both unless your application actually needs both.

## Presentation without a product profile

For a bare prompt, set `editor.prompt[0]=0` after `ps_editor_init`; the configurable
`PS_EDITOR_PROMPT_SUFFIX` supplies `> ` or a custom symbol. With `editor.theme=255`
(the initial value), `PS_EDITOR_PROMPT_STYLE="\033[1;93m"` selects a bold bright-yellow
prompt. Named palettes are optional; the semantic default remains unchanged.

Full sessions use `ps_set_prompt` / `ps_set_prompt_parts`; the caller supplies the
text and style. DeviceView independently offers category-column or section rows,
plain rows, inside-table/vertical tabs, section borders, inline edits and dialogs.
See [Options](Options.md) and [Workspace](Workspace.md). A presentation choice is
not permission to change command semantics or drop validation.

## Formatting and output semantics

`ps_terminal_format` supports signed `%d`, including `INT_MIN`, alongside `%u`,
`%02u`, `%s`, and `%%`. It returns required length and terminates bounded output.
`ps_terminal_number_pattern(write, context, pattern, value)` substitutes the signed
value for **every** `%`; `%%` substitutes twice. This explicitly preserves that
small formatter behavior without a new general printf engine.
These APIs use native `int`, not a fixed 32-bit type. On targets with 16-bit `int`,
they do not replace a device's 32-bit formatter for values outside that range;
retain that consumer formatter rather than narrowing its arguments.

These terminal helpers emit trusted application bytes. `ps_write` is sanitized
text, not a drop-in raw ANSI writer. Keep raw output at an explicit callback boundary.
Broadcast loops over selected sessions in the consumer; there is no hidden global
current-session pointer. Connection close and reconnect stay with the adapter.

## Evidence and limits

The C selection tests verify zero-history editing, cursor/draft preservation,
bell behavior, yellow bare prompts, signed boundaries and pattern substitution.
On the tested x86-64 compiler with 32-byte lines and undo disabled, editor storage
was 248 bytes with no history versus 344 bytes with two entries (other capacities
left at defaults). These are `sizeof` results, not AVR stack or runtime totals.
The new signed formatter uses storage derived from integer width rather than a
fixed decimal length. Header-only helpers add executable code only when used.

[Source lineage and explicit behavior dispositions](https://github.com/atomicdeploy/portable-shell/blob/main/docs/Attribution.md)

