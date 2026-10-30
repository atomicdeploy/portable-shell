# Rich text output

portable_rich.h is an optional, header-only C99 renderer built on portable_terminal.h. It uses caller-supplied byte callbacks and no allocation, OS APIs or external libraries. It is an additional composable API; existing shell and table implementations are preserved.

## Capabilities

| Feature | Support and boundary |
| --- | --- |
| Color | RGB foreground/background, nearest palette conversion to 256 or 16 colors; explicit no-color output. The host selects capabilities. No OS/environment probing in core. |
| Styles | Bold, dim, italic, underline, strike, reverse and blink SGR. Actual appearance depends on the terminal; blink is never enabled implicitly. |
| Layout | Cell-based clipping, padded fields, word wrapping, left/center/right/full alignment and indented multiline output. Width must be at least two cells. |
| Unicode | Strict UTF-8 decoding, replacement of malformed bytes, preservation of CJK/emoji text, common wide and combining scalar widths. Optional width callback for terminal policy. |
| Markup | Nested style and hex-color tags, bounded eight-level stack, literal-bracket escaping. |
| Documents | Headings, unordered lists, quotes and fenced code blocks. A limited Markdown subset, not CommonMark. Lines are bounded to 511 bytes. |
| Syntax | Small C-like/JSON lexical highlighter: strings, numbers, identifiers and line comments. Not a grammar-aware language engine. Source formatting is preserved. |
| Tables, columns, progress, logs | Continue using existing table, report, dashboard, meter and monitor APIs. New text fields can be composed through the same byte transport. |
| Pretty printing / tracebacks | Producers can send preformatted structures to the lexical renderer. Automatic object inspection, JSON reformatting and stack unwinding are not implemented. |

Unicode width is a conservative scalar policy, not full Unicode grapheme segmentation, bidi shaping or a complete versioned width database. ZWJ emoji sequences can occupy a different width in a particular terminal. The new renderer does not silently change legacy sanitization in existing controls or the monitor.

## API example

    #include "portable_rich.h"
    ps_rich text = {write_bytes, context, PS_COLOR_RGB, 0, 0};
    ps_rich_color(&text, 0x56b6c2, 0);
    ps_rich_attributes(&text, PS_TEXT_BOLD);
    ps_rich_wrap(&text, "Deployment status", 60, PS_ALIGN_CENTER);
    ps_rich_reset(&text);
    ps_rich_markup(&text, "[#98c379][bold]Ready[/][/]");

Use ps_rich_text for untrusted text: it validates UTF-8 and neutralizes C0/C1 controls. ps_rich_raw and ps_rich_literal intentionally bypass validation and are for trusted application-owned output only.

ps_rich_field clips to terminal cells and pads to a requested width. ps_rich_wrap_indented adds the requested left margin to every line without cursor escape sequences. ps_rich_markup returns a count of nesting errors/unclosed tags; unknown tags remain literal. Supported tags are bold, dim, italic, underline, strike, reverse, blink, #RRGGBB and / (close one level). Double opening brackets escape a literal bracket. Markup does not expand emoji aliases: supply UTF-8 directly.

Set PS_RICH_MARKUP=0, PS_RICH_DOCUMENT=0 or PS_RICH_SYNTAX=0 to exclude those helpers. Color capability is runtime-selectable; unused inline functions are not emitted. PS_COLOR_NONE suppresses all generated ANSI in these helpers.

## Preview and evidence

The image is generated from actual C renderer bytes, replayed onto a deterministic terminal surface. It uses Agave with pinned Noto CJK and Emoji fallback fonts; those are host-only preview dependencies, never firmware dependencies. The test renderer models selected styles; exact terminal font synthesis can differ.

![Rich text feature preview](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/rich-text.png)

Tests cover malformed UTF-8, CJK clipping, combining widths, escape neutralization, color conversion, no-color mode, alignment, wrapping, markup nesting, document blocks and lexical output. CI also builds with markup/document/syntax excluded and checks preview freshness.

## Run the hosted example

    cmake -S . -B build -DPORTABLE_SHELL_RICH_EXAMPLE=ON
    cmake --build build --target portable_shell_text
    build/PortableShellText --truecolor

On Windows the executable has an .exe suffix. No arguments defaults to plain output for pipelines. Select --16 or --256 explicitly to test palette conversion. This adapter alone uses stdio; the renderer remains freestanding.
