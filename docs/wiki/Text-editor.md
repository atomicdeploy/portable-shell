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
