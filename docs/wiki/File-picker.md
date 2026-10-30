# 📁 Native directory browser and tree picker

The optional Windows/POSIX hosts now enumerate **real directories**. The freestanding
core still has no filesystem calls, allocator or platform dependencies.

| Command | Result |
| --- | --- |
| `files PATH` | Directory tree on the left; selected directory listing on the right |
| `files --tree PATH` | Expandable Unicode tree, including files |
| `files --list PATH` | Single directory listing; Enter opens folders, Backspace goes to parent |
| `files --nerd PATH` | Split view with Nerd Font folder/file/link icons |
| `files --emoji PATH` | Split view with Unicode folder/file/link emoji |
| `files --ascii PATH` | ASCII icons and normal terminal border policy |
| `ls PATH` | Plain directory listing, including entry type; works in scripts |

PATH defaults to the host working directory and can be quoted. `PortableShell --files`
starts directly in the native browser. Use `--ascii` / `--no-color` host options for
ASCII/monochrome terminal operation. Native commands attach only to the local host
session, not serial or Telnet clients. Embedded clients retain the original in-memory
`files` command and may attach their own provider. `edit` still edits session notes.

The adapter is read-only: choosing a file returns/prints its absolute selected path;
it does not execute or edit the file. Loading and transactional saving are separate
application responsibilities. Directory entries are loaded when needed, sorted with
folders first. Automatic traversal skips symbolic links and Windows reparse points;
they remain visibly marked, unavailable entries. Permission/enumeration failures are
reported. There is no filesystem watcher: reopen to refresh a snapshot.

Tab changes pane focus. ↑/↓, Home/End, Page Up/Down and the wheel navigate. ←/→ folds
the tree; Enter/Space toggles directories. Click selects; clicking a tree folder icon
expands/collapses it. Enter on a file selects it; Esc/Ctrl-C cancels. Each pane scrolls
independently. Narrow terminals use listing mode in place of a squeezed split view.

## Icons and screenshots

Unicode is the default and needs no patched font. Nerd mode emits the standard PUA
folder/open-folder/file/link symbols and requires a Nerd Font in the terminal.
Emoji uses two terminal cells per symbol; font and color rendering depend on the
terminal. ASCII fallback is automatic when Unicode is disabled. The CI images use
pinned Agave, Agave Nerd Font Mono and monochrome Noto Emoji fonts. All images below
come from actual Windows/POSIX provider enumeration of checked-in **files on disk**.

![Directory tree](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/native-tree.png)
![Directory listing](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/native-list.png)
![Split view](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/native-split.png)
![Nerd Font icons](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/native-nerd.png)
![Emoji icons](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/native-emoji.png)

## Provider API

`ps_file_picker_open` remains available for a prebuilt caller-owned tree.
`ps_file_browser_open` additionally takes a lazy loader:

```c
/* Provider keeps nodes[128] and names alive; parent precedes its children. */
unsigned load_children(void *context, unsigned parent, unsigned count);
void selected(ps_session *session, void *context, unsigned node_index);
ps_file_picker_style(s, PS_FILE_SPLIT, PS_FILE_ICONS_NERD);
ps_file_browser_open(s, title, nodes, initial_count, selected, load_children, context);
```

The loader appends children, preserving existing indices and names, and returns the
new count or zero on failure. The core validates the resulting tree. It must not
reenter the session. Nodes can have `PS_FILE_DIRECTORY`, `PS_FILE_DISABLED` and
`PS_FILE_LINK` flags. Each directory is loaded once per open session. The selected
callback runs after picker close, receives the original node index, or `PS_FILE_ROOT`
on cancellation. Explicit API close is silent. Title/tree/context remain borrowed.

Limits: 128 nodes across the current expanded snapshot, 16 ancestor levels; native
paths at most 1023 UTF-8 bytes and names at most 511. A full snapshot displays an
explicit limit notice: reopen a narrower PATH to browse beyond it. `ls` streams all
entries without the snapshot node limit. Empty folders work. Batch `files` refuses
interactive operation without escape output. The browser itself adds no OS calls;
`host/filesystem.h` contains the Windows/POSIX adapter and is never linked into MCU
core-only builds. A host snapshot occupies about 200 KiB of static storage, separate
from session storage; no filesystem adapter heap allocation is used on Windows.
