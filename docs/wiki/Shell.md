# Shell


```text
task add "Review the firmware interface"
task done 1
task rename 1 "Review the UART interface"
task list
set target arm32
get target
text slug "Build a useful little terminal"
text upper 'quoted arguments'
calc 1024 * 32
hash "portable bytes" --rounds 100000
board jobs
bind F4 "board variables"
display color mono
display glyphs ascii
status
```

`help` groups registered commands under category headings. `help --compact` retains the category column layout. `help NAME`
shows a command's usage. `args` exposes parsed argument boundaries and byte lengths.
`echo`, `get`, `unset`, `jobs cancel`, `history clear`, `clear`, and `exit` are also
available. `text` supports ASCII upper/lower/slug transformations and whitespace trim.
`calc` performs checked signed 32-bit integer arithmetic. FNV-1a is a noncryptographic
32-bit hash of the supplied UTF-8 bytes concatenated for the requested round count.
Jobs execute bounded chunks when the host calls `ps_tick`; they do not sleep or block
input. Batch EOF drains pending work.

Parsing supports single/double quotations, empty quoted arguments, concatenated
quoted fragments, backslash escapes (`\n`, `\r`, `\t`, escaped punctuation), and
comments beginning at a token boundary. `ps_option` recognizes `--name=value`,
`--name value`, boolean `--name`, and the `--` option terminator.

| Prompt keys | Action |
| --- | --- |
| Tab, repeated Tab | Common-prefix completion, candidate list, candidate cycling |
| Up / Down, Ctrl+P / Ctrl+N | History with draft restoration |
| Ctrl+R | Incremental reverse history search; repeat for older match |
| → or End at end, Ctrl+F / Ctrl+E | Accept the visible muted suggestion; Enter is still required to execute |
| Left / Right, Ctrl+A / Ctrl+E | Cursor movement, start/end |
| Ctrl+Left / Ctrl+Right, Alt+B / Alt+F | Word movement |
| Backspace / Delete | Delete a UTF-8 code point |
| Ctrl+U / Ctrl+K / Ctrl+W, Ctrl+Y | Clear/kill line, kill suffix, kill word, yank |
| Ctrl+C | Clear input and cancel an active job |
| Ctrl+D | Delete under cursor, or exit on an empty prompt |
| Ctrl+L | Clear screen |
| F1 / F2 / F3 / F4 | Commands / workspace / jobs / color toggle (unless rebound) |

Completion understands quoted prefixes, completes registered commands, subcommands,
variable keys and `--rounds`, and supports caller-provided completion callbacks.
Ghost suggestions are enabled by default, with no special build flag: newest matching
history first, then registered command/argument completions. Only a suffix is offered;
quoting that requires rewriting the typed prefix remains a Tab operation. End within
the line first moves to its end. `--no-suggestions` (embedding: `PS_NO_SUGGESTIONS`)
disables previews while retaining Tab. Completion callbacks also serve previews and
must enumerate stable candidates without changing application or device state.

`bell` or Ctrl+G emits BEL (byte 7), as do invalid cursor/history operations,
no-match Tab and full input buffers. `--no-bell` (`PS_NO_BELL`) mutes it. The core
API is `ps_ring_bell(session)`; it uses the supplied byte sink, including Telnet or
serial. The receiving terminal controls sound/visual bell preferences. Batch mode
emits neither bells nor ghost previews. The original optional `ps_bell(PortableShell*)`
compatibility API retains its explicit raw-byte behavior.

Bracketed paste is inserted without executing pasted newlines. Escape sequences may
arrive one byte at a time; a standalone Escape resolves after a 60 ms tick timeout.


## Custom prompts

Prompts support colored segments, UTF-8 symbols, and up to four lines. Text is
copied into the session (255 bytes total, at most 16 parts). Only LF is accepted
as a control character; use styles rather than embedding ANSI escapes.

Try these commands in the hosted shell or over Telnet:

~~~text
prompt success "operator@workbench " muted "/configuration\n" command "❯ "
prompt heading "controller\n" success "-> "
prompt reset
~~~

Available styles: `default`, `heading`, `success`, `warning`, `error`,
`green`, `muted`, `command`, and `blue`. They follow the selected palette;
monochrome mode omits color. Custom symbols are ordinary text, so callers can
choose ASCII symbols for terminals that need them. No font is required by the core.

~~~c
ps_prompt_part parts[] = {
    {"operator@workbench", PS_STYLE_SUCCESS},
    {"  /configuration\n", PS_STYLE_MUTED},
    {"╰─❯ ", PS_STYLE_COMMAND}
};
ps_set_prompt_parts(session, parts, 3);
~~~

`ps_set_prompt(session, "host\n> ", PS_STYLE_PROMPT)` remains available for a
single style. Passing NULL restores the original prompt. Both setters reject
invalid input without replacing the current prompt. Host callbacks can rebuild
the parts when hostname, directory, connection, or status changes; the core does
not infer OS state.

The final line owns the editor and suggestions. Prompt lines are clipped to
terminal width, and excess leading lines are hidden in short viewports.
Repainting returns to the existing prompt instead of appending another header.
Switching to fewer lines erases stale content. Command submission starts a new
prompt beneath its output; non-interactive output contains no prompt.

Actual-output fixtures (dark Agave, generated in CI):
[Colored](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-colored.png) ·
[Multiline](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-multiline.png) ·
[ASCII monochrome](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-ascii.png).

## Contextual help

Run help for grouped command descriptions, help segments for a detailed
command page, or help --compact for the original category/syntax listing.
Descriptions wrap beneath their own column; narrow layouts move long labels
onto a separate line. Plain output contains no terminal escapes.

Attach borrowed immutable ps_help_document records using
ps_set_help_documents(session, documents, count). Each record can supply a
description plus argument, option and example arrays of ps_help_item.
Keep the records and their strings alive until detached with
ps_set_help_documents(session, 0, 0). Existing ps_command initializers are
unchanged. Only registered commands are listed; undocumented commands retain
their usage text. Metadata describes callbacks; it does not add parser flags.

Build with PS_HELP_DETAILS=0 to omit descriptions, metadata storage and
detailed sections while retaining grouped/compact help and usage. Colors follow
the existing semantic roles and terminal settings. Wrapping reuses the shell UTF-8 decoder and cell-width rules; terminal-dependent emoji clusters may still vary in width.

The [command page](https://github.com/atomicdeploy/portable-shell/blob/main/previews/help-command.png),
[registry](https://github.com/atomicdeploy/portable-shell/blob/main/previews/help-registry.png)
and [monochrome wrapped page](https://github.com/atomicdeploy/portable-shell/blob/main/previews/help-plain.png)
are actual output replays rendered with Agave.

## Dotted-leader reports

Use status --report for the current session's live storage and activity report.
The original single-line status output remains the default.

For application reports, call ps_print_report(session, rows, count, style).
Each ps_report_row contains a label, optional muted annotation, optional
right-aligned value and flags. PS_REPORT_SECTION styles section headings;
PS_REPORT_GAP inserts separation before a section; PS_REPORT_NUMBER uses the
numeric semantic color. Strings are borrowed only during the call.

PS_REPORT_DOTS produces spaced dot leaders by default. PS_REPORT_DENSE selects
continuous dots; PS_REPORT_SPACES removes dots. Terminal color and Unicode
settings still apply, and non-interactive output contains no escapes. Reports
are borderless output, not editable tables. Long content wraps without losing
text; values move below labels when both cannot fit on one line.

PS_REPORT=0 excludes the renderer and status --report; the API remains a no-op
returning zero. No per-session storage, heap allocation or host API is added.
Help and reports share text-flow, UTF-8 and semantic-color primitives.

[Schema fixture](https://github.com/atomicdeploy/portable-shell/blob/main/previews/report-leaders.png)
and [live session report](https://github.com/atomicdeploy/portable-shell/blob/main/previews/report-status.png)
are rendered from actual emitted bytes with Agave. Schema rows are demonstration
data, not a database connection.

## Framed forms

The optional ps_form API provides framed text questions, radio-style choices,
validation and sequential steps. Call ps_form_open with a ps_form descriptor.
Each ps_form_field supplies a label, kind, writable value buffer and capacity.
Text fields support minimum/maximum Unicode code-point counts; choice fields
supply borrowed strings. A validator may return a borrowed error message or
NULL to accept. The done callback receives accepted=1 after the last step, or
accepted=0 on cancellation.

All descriptors, choices, buffers and callback context must remain alive until
the form closes. Enter validates before changing a value. Accepted steps write
their caller buffers immediately; cancel leaves earlier accepted steps intact,
so applications requiring atomic updates should use draft buffers and commit
only in the successful done callback. Rejected/current drafts never overwrite
the caller's value. The active input uses the existing PS_VALUE editor buffer.
This is not a password-input API.

Text supports insert/delete, Home/End, Ctrl-U and horizontal scrolling. Choices
support Up/Down and Home/End, with a viewport for longer lists. Mouse clicks
position the text cursor or select choices; wheel events navigate choices.
Pasted newlines cannot accept a step. Esc or Ctrl-C cancels. The preceding
accepted answer is displayed above the next question when height allows;
all accepted answers remain in the caller buffers.

Forms use an alternate screen and require an interactive terminal at least
20 columns by 8 rows. Smaller resize events retain state until the viewport
becomes usable again. Non-interactive open returns zero immediately; consumers
should supply arguments or another input provider in batch mode. A competing
workspace cannot take over the form's shared editor. Close restores cursor,
mouse reporting and the original screen.

PS_FORM=0 removes form state and rendering; public entry points become inert.
PS_MOUSE=0 independently removes mouse support. Existing semantic color,
ASCII/Unicode and synchronized-output settings apply. No allocation or OS API
is introduced. tests/form_checks.c is a complete descriptor/callback example
and drives the same public byte-input API used by a host.

Actual-output Agave previews:
[validation](https://github.com/atomicdeploy/portable-shell/blob/main/previews/form-validation.png),
[choices](https://github.com/atomicdeploy/portable-shell/blob/main/previews/form-choice.png),
[steps](https://github.com/atomicdeploy/portable-shell/blob/main/previews/form-steps.png).

## Multi-panel dashboard

The control provider can be presented as simultaneous panels instead of tabs:

~~~c
ps_panel_attach(session, &panel);
ps_panel_set_layout(session, PS_PANEL_GRID);
ps_panel_open(session, 0);
~~~

Each ps_control_page becomes a bordered panel. At 90 columns and 24 rows or larger,
up to four panels appear in a two-column grid. Narrower terminals use one column;
shorter terminals use one row. Tab and Shift+Tab move focus through all pages,
including pages outside the current visible group. Very small terminals fall back
to the existing table layout. This is a responsive grid, not an arbitrary nested
window compositor.

Use ↑/↓ to select a row, ←/→ to adjust by its declared step, Home/End for bounds,
Space for toggles, Enter for inline editing, F2 for the existing dialog, and Esc
to cancel an edit or close the view. Clicking a row focuses it; the wheel moves
the selection. Oversized panels scroll to keep selection visible and show a range
on the lower border. The footer stays at the bottom of the terminal.

All reads, writes, validation, pending/locked handling and revision checks use the
same provider path as the tabbed view. The host supplies observed values and can
call ps_panel_refresh after changes. Draft edits do not write until Enter.

PS_PANEL_TABS remains the default. The demo command controls dashboard selects
the grid; controls open returns to tabs. PS_DASHBOARD=0 removes the dashboard
renderer and state; requesting PS_PANEL_GRID then returns zero.

The following previews replay real output from a simulated provider, not hardware:
[dashboard](https://github.com/atomicdeploy/portable-shell/blob/main/previews/dashboard.png)
and [inline editing](https://github.com/atomicdeploy/portable-shell/blob/main/previews/dashboard-edit.png).
