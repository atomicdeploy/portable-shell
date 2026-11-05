# .env editor and manager

The native Windows and POSIX adapters provide a **local, persistent dotenv editor**.
The command, document model and file operations live in `host/`. They are not part
of the freestanding library, firmware, UEFI application, serial or Telnet sessions.
External command execution remains disabled and is not needed.

```text
build/PortableShell --env examples/environment.env
```

On Windows use `PortableShell.exe`. Inside the shell, `env FILE` opens a file;
`env` opens `.env`. `env create FILE` creates an empty file exclusively and never
overwrites an existing path. `env check FILE` reports record, unparsed-record and
duplicate-key counts without printing values. It returns nonzero for I/O, encoding,
capacity or unsupported-syntax errors. Duplicate keys are warnings, not errors.

## Editing

| Key | Action |
| --- | --- |
| Enter / F2 | Inline / modal edit of a short visible value |
| Ctrl+E | Edit the complete record: key, value, spacing and inline comment |
| Tab | Values / Source order, including comments |
| F4 | Show / hide blank source records (hidden by default) |
| F3 or `/` | Find keys by case-sensitive substring; empty query clears filtering |
| Ctrl+T | Comment / uncomment an assignment, including quoted multiline records |
| Ctrl+U / Ctrl+D | Move one source record upward / downward |
| Ctrl+N / Delete | Add an entry / confirm deletion |
| Ctrl+Z | One-step undo / redo |
| Ctrl+S | Save the document to disk |
| Ctrl+L | Reload from disk after confirmation |
| Ctrl+R | Reveal / mask names resembling credentials |
| `[` / `]` | Previous / next 128-record window for large documents |
| F1 | Scrollable keyboard reference |
| Esc / Ctrl+Q / Ctrl+C / q | Close; modified documents offer Save, Discard or Keep editing |

Both views have a dedicated narrow **Line** column containing original physical
source line numbers, separate from **Key** and **Value / Source**. Filtering keeps
these source numbers rather than renumbering the visible rows.

The existing table supplies mouse selection, scrolling and inline/dialog editing.
The record editor supplies wrapping, cursor movement and its own save/discard/cancel
flow. **Saving in the record editor updates the document draft**; Ctrl+S in the
table persists the file. Values longer than a table cell can hold are read-only in
the table and use Ctrl+E; they are never silently truncated by an inline edit.

Commented entries are muted and prefixed with `#`. Duplicate occurrences have
amber keys and `!`; edits address a specific record, never every matching name.
Values show trailing comments in muted text alongside the editable value. Source
order highlights keys, `export`, literal strings, numbers, booleans and comments
separately. Quoted or escaped `#` characters remain value text; an unquoted `#`
starts a comment only at the beginning of a value or after whitespace. A `#`
following a closed quoted value also starts a comment. Editing a value never
includes its annotation in the edit buffer.

Blank records are hidden by default, including separators before comment groups.
F4 reveals them in Source order; visibility never changes document bytes, physical
line numbers or undo history. A comment block at the start of the document or
after a blank separator can introduce assignments: its last nonempty comment is
highlighted as a **Section**. This is a presentation hint, not dotenv syntax.
Commented-out assignments remain independently toggleable entries, and ordinary
comments following an assignment remain comments. Standalone comments remain
independent records and can be moved or edited in Source order. Moves carry the
record's inline comment and multiline value. Moving a final record without a
newline currently requires adding that newline explicitly in the record editor.

## Preservation and dialect

Untouched source bytes survive saves: order, blank lines, comments, indentation,
spaces around `=`, LF/CRLF endings, a UTF-8 BOM and absence of a final newline.
Recognized assignments use shell-style identifier keys, optional `export`, bare
values, single/double quotes, inline `#` comments and quoted multiline values.
Double-quoted backslash escapes are recognized when locating the closing quote.
Commented multiline assignments require a `#` prefix on every continuation line.

Editors expose **literal dotenv syntax**, including quotes. They never evaluate
`${VARIABLE}`, command substitutions, escape sequences or external commands and
never modify the process environment. Unsupported records are retained and shown
as unparsed text. UTF-8 errors, NUL and terminal control bytes are rejected.
This is not a promise to interpret every dotenv dialect.

The default limits are 65,535 document bytes and 1,024 records, configured with
`PS_ENV_BYTES` and `PS_ENV_RECORDS`. Short inline values use the shared 63-byte
cell capacity. The full-record draft is bounded by `PS_ENV_DRAFT_BYTES` and the
core's separately configured `PS_EDITOR_CAPACITY` (both default to 2,048 bytes).
An oversized record remains intact and the editor reports its limit. The host
model/UI occupies about 415 KiB on x86-64, plus a 64 KiB input staging buffer and
the existing session; these buffers are absent when the host feature is disabled.

## File safety and limits

Saves compare disk bytes against the last load/save and refuse external changes.
They write an exclusively created sibling staging file, flush it, recheck the
original and replace it. Save failures leave the draft available. Windows uses
the original DACL for staging and `ReplaceFileW` for replacement; POSIX preserves
owner, group and permission bits. POSIX extended ACLs/xattrs are not copied.
This is optimistic conflict detection, not a cross-process lock: another writer
can still race the final check. Crash recovery and directory-fsync durability are
not guaranteed. A leftover `.portable-shell.tmp` blocks another save rather than
being overwritten. Symlink/reparse-point files and non-regular files are refused.

Masking uses key-name heuristics, not secret discovery. Ctrl+R reveals values;
Ctrl+E explicitly displays raw source. Session memory contains the original file.
Previews use demonstration values, never production configuration.

## Build and reuse

`PORTABLE_SHELL_HOST_ENV=ON` is the native-host default; set it to `OFF` to omit
the manager, command and file backend. It is independent of `PORTABLE_SHELL_HOST_EXEC`.
Host consumers can reuse [the document model](../../host/env_document.h),
[file backend](../../host/env_file.c) and [UI controller](../../host/env_manager.h).
Call `env_manager_dispatch()` after input/ticks; mutations requested by table
shortcuts are dispatched outside renderer callbacks. A custom save callback can
persist to a different host-owned store.

The reusable core additions are optional panel shortcuts, selection access,
borrowed panel column/footer text and inactive/attention presentation flags.
`ps_panel_chrome.text` supplies display-only text and per-byte `PS_INK_*` roles
for text/label cells, independently of the editable value. It uses the existing
UTF-8 clipping, semantic colors and selection background. `PS_PANEL_TEXT=0`
omits token rendering (the editor remains usable with plain cells);
`PS_PANEL_TEXT_BYTES` bounds its temporary display buffer, default 256 bytes.
`PS_PANEL_KEYS=0` omits the shortcut/chrome integration; the host manager requires it.
The parser and application strings never enter the core.

`env_checks` exercises parsing, exact preservation, editing, ordering, file
conflicts and UI navigation. The normal CI preview rebuild generates six images
from real emitted terminal bytes. See [the gallery](Gallery.md).
