# Terminal interaction and compatibility

PortableShell provides byte-oriented building blocks with ncurses/readline-like behavior. It does not expose either library's ABI and is not a drop-in replacement. It retains the existing shell, event editor, tables and transports.

## Implemented foundations

| Area | Available behavior |
| --- | --- |
| Prompt | Cursor/word movement, insertion/deletion, history and reverse search, completion and muted suggestions, bindings, kill/yank, optional extended keys |
| New undo | Ctrl-_ undo; Alt-_ redo; full-shell PS_KEY_UNDO / PS_KEY_REDO events. Same journal engine in full shell and event editor |
| Paste | Full shell and event editor protect bracketed paste; pasted line endings become spaces rather than submitting commands |
| New retained screen | Caller-owned cell buffers, clipped window views, borders, text, scrolling, pad viewport copy, changed-cell presentation, explicit invalidation |
| Existing TUI | Editable tables, menus, dialogs, panels, progress, logs, file picker, editor, mouse events and resize APIs |
| Output | Explicit color capabilities, ASCII/Unicode choices, byte callbacks, optional synchronized output; host adapters own native I/O |

## Undo capacity and lifetime

PS_UNDO_DEPTH and PS_EDITOR_UNDO_DEPTH default to 2; set either to zero to remove that journal, or increase it for longer history. Capacity macros must agree across translation units. The full shell adds (depth+1)*PS_LINE bytes plus cursor slots and journal metadata; the event editor uses (depth+1)*(PS_EDITOR_CAPACITY+1). Neither allocates dynamically.

Snapshots preserve cursor positions. New edits after undo discard the redo branch. Full bracketed pastes form one undo step. Submission/cancel/clear discard the journal. UTF-8 sequences are not checkpointed halfway through arrival; the small event editor retains its existing ASCII editing policy. Typing is recorded per completed character, not coalesced into word-sized transactions. Custom shell bindings take precedence over built-in undo shortcuts.

The generic portable_undo.h API can also serve other bounded editors with caller-owned storage. Its buffers must remain alive while bound.

## Retained screen API

Include portable_screen.h only in consumers needing a cell canvas. No screen buffer is added to the normal session. The component reuses portable_rich.h for UTF-8 scalar widths, styles and color conversion.

1. Supply two distinct non-overlapping ps_cell arrays to ps_screen_init.
2. Obtain clipped rectangles with ps_screen_window.
3. Draw with ps_window_text, ps_window_put, ps_window_fill and ps_window_border.
4. Scroll a view using ps_window_scroll. Copy a viewport from a different canvas with ps_window_blit; use a larger source canvas as a pad.
5. Call ps_screen_present to send changed cells. An unchanged frame emits zero bytes.
6. After external terminal writes, call ps_screen_invalidate to force a full repaint.

The host supplies terminal dimensions, input events, frame timing and alternate-screen/cursor lifecycle. The callback must synchronously accept all bytes; queue/handle backpressure in the adapter. This is cursor-addressed ANSI output, even when the color capability is NONE. Use the existing plain logging/report paths for redirected output.

A view does not own storage. Reinitialize explicitly on resize using arrays sized for the new dimensions. At most 512 columns by 256 rows are accepted. Full-cell backing storage trades RAM for reliable diffing: two arrays consume 2*columns*rows*sizeof(ps_cell), plus the small descriptor. This is opt-in, particularly for firmware.

Wide scalar glyphs occupy two cells. Overwriting a continuation clears the original wide glyph, and clipping never writes a partial wide glyph. Combining clusters, bidirectional layout and arbitrary ZWJ sequences are not supported by this canvas. Keep the canvas smaller than the physical terminal to avoid lower-right autowrap, or manage autowrap in the host.

## Remaining deliberate gaps

- No terminfo/termcap database, automatic terminal probing, OS signal/job-control layer or native raw-mode setup in the core.
- No GNU Readline ABI, inputrc parser, full vi command mode, multi-entry kill ring/yank-pop, numeric key arguments or keyboard-macro recorder.
- No ncurses ABI, automatic overlapping-panel stack, grapheme-aware complex-text canvas or terminal-specific scroll-cost optimizer.
- History persistence and application-specific completion remain host callbacks; the core does not read files or environments.
- Existing TUI surfaces continue using their current renderers. Consumers may adopt the retained canvas incrementally; this change does not replace tested layouts wholesale.

These are explicit unsupported areas, not claims of full ncurses/readline parity.

## Actual-output preview

This deterministic fixture uses the new screen API, including a viewport copied from a 40-row pad. It demonstrates rendering, not live telemetry.

![Retained terminal screen](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/retained-screen.png)

## Local footprint checkpoint

On x86-64 Windows with the default capacities after the contextual table layout addition, the host executable is 130,560 bytes on disk; status reports 39,448 caller-owned session bytes and zero core heap bytes. The event editor occupies 6,928 bytes. A cell occupies 24 bytes, so two 96 by 24 canvas arrays occupy 110,592 bytes. These are binary/storage measurements, not process working-set or board-runtime measurements; target ABI, compiler flags and selected features change them.

See [Terminal OSC integration](OSC.md) for window titles, terminal progress and working-directory reporting.

## Mouse ownership and terminal scrollback

The standalone command prompt uses the normal screen and leaves mouse-wheel
scrollback to the terminal. Full-screen tables, menus, forms and file pickers
capture mouse events while they own the alternate screen. The document editor
also uses the alternate screen, but currently remains keyboard-driven.
Table-plus-prompt mode is a full-screen table workspace; its lower prompt does
not turn it into a normal scrollback shell.

Native adapters can query `ps_terminal_state()` and check
`PS_TERMINAL_ALT_SCREEN` and `PS_TERMINAL_MOUSE`. Mouse ownership implies alternate
screen ownership; alternate screen ownership does not require mouse capture.
The Windows host follows this state when configuring `ENABLE_MOUSE_INPUT` and
restores the original console mode on exit.
