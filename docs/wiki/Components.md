# Shared firmware components


The reusable editor, Telnet parser, tokenizer, terminal-size probe and
snapshot/action device view are maintained here. Both the full C runtime and
callback editor are available on capable MCUs. Feature flags, not CPU detection,
select smaller builds. The shared device table defaults to rich presentation.
See [integration contracts](Integration.md), [profiles and options](Options.md), and
[portability](Portability.md) for host, AVR, ARM and serial integration boundaries.


## Progress and values

See [meter controls](Meters.md) for line/block styles, panels, labels, read-only progress and adjustable targets.

See [file tree picker](File-picker.md), [menus](Menus.md) and [document editor](Text-editor.md) for reusable workspace callbacks.

## Seven-segment display

The reusable C99/C++11 component is `portable_segments.h`. It renders through
a byte callback without allocation, clocks, OS calls or external libraries.

~~~text
segments 12:48.6
segments -23.5 --red
segments 0123456789 --ascii --mono
segments 1234 --large
~~~

The command uses current terminal Unicode/color settings. Displays that do not
fit are rejected before drawing. It also works in batch mode without ANSI color
or synchronization escapes.

~~~c
#include "portable_segments.h"
unsigned short cells[PS_SEG_MAX_CELLS];
ps_segment_style style = ps_segment_default_style();
int count = ps_segment_parse("12:48.6", cells, PS_SEG_MAX_CELLS);
if (count > 0)
    ps_segment_render(cells, (unsigned)count, &style, 80, write_bytes, context);
~~~

Horizontal segment length is 1–4 cells; vertical length is 1–3. Configure
`foreground` and `inactive` with ANSI 256-color indices, `show_inactive`,
`unicode`, and `color`. Monochrome leaves inactive segments blank.
The return value is the emitted height; invalid input or insufficient width
returns zero without output. Each row includes trailing padding and ends in CRLF.

Digits, spaces, `-`, `_`, `:`, decimal points, and the approximate letters
`A b C c d E F H L P U o r n` are supported. Unsupported text is rejected.
For custom glyphs, pass masks directly using `PS_SEG_A` through `PS_SEG_G`,
`PS_SEG_DP`, and `PS_SEG_COLON`. At most 32 cells are accepted. Segment masks
describe display state; the component does not drive hardware pins.

For live telemetry, the consumer supplies observations, keeps the cell count
fixed, repositions to the same origin, and calls render again. The host owns
timing and positioning. Synchronize the surrounding frame where supported.
This is a read-only display; application controls own value changes.

![Seven-segment display](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/seven-segment.png)

Actual emitted-byte fixture: amber clock-style value, compact red decimal value,
and ASCII monochrome digits. Observations are simulated, not hardware readback.
