# 📁 File tree picker

`files` opens the current session's **in-memory** resources. Expand Session and open
notes.txt to edit the same saved notes used by `edit`. It does not enumerate your disk.

The reusable picker accepts a provider-owned file tree with no filesystem APIs or
allocation in the core. Disk, flash, remote and virtual providers can use the same
control. A provider must enumerate resources, handle access restrictions and perform
file reads/writes itself. This change does not install a native disk provider.

```c
static const ps_file_node nodes[] = {
    {"Documents", PS_FILE_ROOT, PS_FILE_DIRECTORY},
    {"notes.txt", 0, 0},
    {"Configuration", PS_FILE_ROOT, PS_FILE_DIRECTORY},
    {"device.conf", 2, PS_FILE_DISABLED}
};
/* on_pick(ps_session *, void *context, unsigned index) */
ps_file_picker_open(s, "Files", nodes, 4, on_pick, context);
```

The tree can have multiple roots. Parent indices must precede their children and
refer to directories. Nodes may be interleaved: rendering traverses depth-first.
Names/tree/title/context are borrowed until close and must remain immutable while
open. Limits are 128 nodes and 16 ancestor levels; invalid/cyclic trees are rejected.
For a changed directory snapshot, close and reopen with fresh nodes. Empty folders
are valid. This bounded snapshot API does not provide lazy enumeration or filtering.

- ↑/↓, Home/End and Page Up/Down navigate; the viewport follows selection.
- → expands a directory; ← collapses or selects its parent.
- Enter/Space toggles a directory. Enter on a file closes the picker and invokes
  the callback with its stable original node index.
- Mouse click selects a row; wheel changes selection. Enter activates it.
- Esc/Ctrl-C cancels and reports `PS_FILE_ROOT` to the callback.
- API `ps_file_picker_close` closes silently; `ps_file_picker_active` returns node
  count or zero. The callback can safely open an editor after the picker closes.
- Disabled entries cannot activate. Pasted text cannot activate files. Batch mode
  refuses the interactive picker without emitting escape sequences.

The screenshot is an emitted-byte replay using a **simulated provider tree**.

![File tree](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/files.png)
