# Progress and value controls

`include/portable_meter.h` provides a header-only C99/C++11 control with no allocation,
OS calls or required runtime library. Presentation and input permissions are independent.

| Option | Values | Suggested default |
| --- | --- | --- |
| Track | `PS_METER_LINE`, `PS_METER_BLOCKS` | Line |
| Text | `PS_METER_TEXT_ABOVE`, `PS_METER_TEXT_INSIDE`, `PS_METER_TEXT_NONE` | Above |
| Panel | `panel = 0` or `1` | Off |
| Title / subtitle | Borrowed strings or NULL | Optional |
| Terminal | `unicode`, `color` independently | Host capability |
| Interaction | `adjustable = 0` or `1` | Read-only |
| Readback + target | `show_requested = 1` | Off |

![Meter variants, emitted-byte replay with simulated observations](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/meters.png)

```c
#include "portable_meter.h"
ps_meter level = {0, 100, 64, 80, 5, 1, 1};
ps_meter_style style = {60, PS_METER_BLOCKS, PS_METER_TEXT_INSIDE,
                        1, 1, 1, "Output", "Observed / target"};
/* write_bytes uses ps_terminal_write's size_t count signature. */
ps_meter_render(&level, &style, write_bytes, transport);
ps_meter_adjust(&level, PS_METER_INCREASE, request_value, device);
```

Route focused left/down and right/up keys to DECREASE and INCREASE; Home/End to
MINIMUM/MAXIMUM. Route track clicks/drags through `ps_meter_position` with a
zero-based cell position. Input routing/focus belongs to the containing application;
the component does not capture input from another control. A rejected callback
leaves the requested value unchanged. A NULL callback updates only the local draft.

Requests never change `observed`. Set that field from application readback, then
render again. For a purely local slider, the host can explicitly copy accepted
`requested` into `observed`. For progress, keep `adjustable = 0` and update observed
from work completion. For both, enable adjustment and show_requested. No fabricated
ACK, persistence, or hardware state is implied.

Ranges use uint32_t; maximum must exceed minimum. Requests clamp to endpoints,
step defaults to one, and arithmetic avoids overflow at UINT32_MAX. Rendering
accepts 8..240 columns inclusive of panel borders. Every row occupies that width;
titles and subtitles are clipped, control-stripped ASCII under the existing terminal
text policy. Border/track glyphs can be Unicode. Narrow widths clip inline labels;
use above-text placement or a wider control when both values need to remain visible.
The host supplies viewport space and positions the cursor before drawing. No heap;
render scratch storage is approximately 820 bytes, so tiny-stack consumers can use
`ps_meter_cell` and `ps_meter_fraction` directly for a single-row display.

Device table `Row::meterStyle` selects LINE or BLOCKS (LINE remains default).
`Row::meter`, `Row::editable`, pending/readback protection and inline editing retain
their existing meanings. Panels and multiline labels use the standalone renderer;
they do not silently expand the existing fixed-height table rows. This does not
change the shell's existing Jobs page layout.
