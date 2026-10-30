# 📝 Text editor


`edit` (or host `--edit`) opens session notes in the alternate screen. These notes
are saved **in memory for the current session**, not to disk. Reopening `edit`
loads the last saved notes. Session shutdown loses them.

- Word wrapping follows the terminal width, with character wrapping for long
  words. Wrapping does not insert newlines in saved text.
- Arrow keys move through UTF-8 characters and visual rows; Home/End (Ctrl-A/E)
  move to visual row boundaries; Page Up/Down scroll by a viewport.
- Enter inserts a newline; Tab inserts a tab displayed at four-cell stops.
  Backspace/Delete remove a character. Bracketed paste accepts multiline text
  and normalizes CRLF to LF without interpreting pasted controls as shortcuts.
- Ctrl-S saves. Ctrl-Q, Esc, or Ctrl-C requests close. Modified documents offer
  **S Save & close**, **D Discard**, or **C/Esc Cancel**. Failed saves keep the
  document open. Discard leaves the last saved data untouched.
- Ctrl-Z toggles the last edit and its redo; a bracketed paste is one undo step.
  This is a single-step undo buffer, not an unlimited history.

Applications can open their own documents with
`ps_editor_open(session, title, initial_utf8, save_callback, context)`. The initial
text is copied. The title and callback context must live until the editor closes.
The callback receives the complete LF document and byte count and returns 1 only
when storage has accepted it; 0 means failure. Implement transactional persistence
in the host/provider when storing files or firmware settings. A null callback
cannot save. `ps_editor_active` reports lifecycle; `ps_editor_text` exposes the
current draft. Neither callback nor transport should reenter the session.

`PS_EDITOR_CAPACITY` defaults to 2048 bytes including the terminator (minimum 128).
The session reserves three such buffers for draft, single undo, and saved session
notes. Configure it consistently for library and consumer builds and allocate
`ps_sizeof_session()` bytes; fixed-size sample host storage may need enlargement
for larger capacities. Oversized initial documents are rejected; a full draft
shows `Buffer full` and retains accepted text. Source text uses LF and tabs;
other ASCII controls are rejected. ANSI batch mode refuses `edit` cleanly.
The reusable core adds no filesystem calls, allocation, or library dependencies.

Set `PS_EDITOR=0` consistently in consumer/library builds to omit document-editor
code and its three text buffers. The open API then returns 0. This is an explicit
feature choice independent of the target processor.

![Text editor](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/document.png)

## Header and border options

`edit --framed` opens with a title header and rounded red border. Applications call
`ps_editor_set_style(s, PS_EDITOR_HEADER | PS_EDITOR_BORDER)` before opening. Each flag
is independent: zero is borderless/headerless; HEADER alone preserves the original
layout. Wrapping, scrolling, page movement and the cursor use the remaining interior
space. Very short terminals temporarily omit the border to keep an editable row.
Settings persist for that session. Save, discard and cancel retain their existing
behavior; the border does not change persistence.

![Framed editor](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/editor-framed.png)

## Binary hex editor

Run `hex` in the demo host to edit a 128-byte session-memory buffer. It is
independent of external command execution. This is a memory demo, not a file
path command. Saving persists the buffer for this session only.

- Arrow keys, Page Up/Down, Home/End navigate the fixed-length buffer.
- Enter two hexadecimal digits to replace a byte; Tab switches to ASCII overwrite.
- Ctrl-S saves through the consumer callback. Esc closes; modified buffers offer
  Save, Discard, or Cancel. A failed save retains the draft.
- Narrow terminals switch from 16 to 8 or 1 byte per row.
- `PS_HEX_EDITOR=0` removes the editor state, implementation, and demo command.

Consumers call `ps_hex_editor_open(session, title, source, length, draft,
capacity, save_callback, context)`. Source and draft must be distinct,
non-overlapping buffers; draft, title, and context remain valid until closure.
The source is unchanged unless the callback writes it. The callback receives all
bytes, including NUL and bytes above 127, and returns 1 only when saving succeeds.
This API performs no allocation or filesystem operations. Editing currently
supports overwrite, not byte insertion/deletion or search.
