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
theme mono
theme ascii
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

