# 🔢 Numbered menus


`menu` (or host option `--menu`) opens a red-bordered table with white labels and
a yellow selection. The width follows the longest item, constrained to the viewport,
and the table is horizontally centered. It uses an alternate screen and restores it
on close. ↑/↓, Home/End and the mouse wheel select; click selects a row; Enter opens
it. Number entry still works. `0` or Esc goes back, Ctrl-C closes, and Backspace edits
the number. Disabled entries cannot run. Paste cannot activate actions.

- `menu --no-numbers`: one column, no numbering column.
- `menu --details`: table on the left, selected-item description and navigation on
  the right. Narrow/short terminals fall back to the centered layout.
- `menu --plain`: original numbered transcript, no table or mouse capture.

Use `ps_menu_set_style` with `PS_MENU_TABLE`, `PS_MENU_NUMBERS`, `PS_MENU_DETAILS`,
or zero for plain mode. Set this before opening; changing style closes the current
menu. `ps_menu_set_description` installs a callback returning borrowed text for the
current item. Descriptions are clipped to the pane width. Numbering remains stable
when hidden. The default callback-free description distinguishes submenus/actions.
A leaf closes menu input before its action runs, allowing another view to open.

`menu 2 1` selects Tools → Terminal status in scripts. Bare `menu` in batch mode
prints choices and returns, without trapping subsequent commands or emitting ANSI.

Custom trees use borrowed `ps_menu` / `ps_menu_item` definitions:

```c
static int show_status(ps_session *s, void *context) {
    (void)context;
    return ps_execute(s, "status");
}
static const ps_menu_item tools[] = {
    {"Status", 0, show_status, 0, 0}
};
static const ps_menu tools_page = {"Tools", tools, 1};
static const ps_menu_item main_items[] = {
    {"Tools", &tools_page, 0, 0, 0}
};
static const ps_menu main_page = {"Main", main_items, 1};
/* In a registered command or application event: ps_menu_open(s, &main_page); */
```

Keep definitions and callback contexts alive while the menu is open. An item has
either a submenu or a callback; `PS_MENU_DISABLED` disables an item. Numbering is
stable and supports 1–999. `PS_MENU_DEPTH` defaults to 8 (configurable 1–32), bounding
both recursion-like trees and accidental cycles without allocation. Applications
may call `ps_menu_choose`, `ps_menu_refresh`, `ps_menu_active`, and `ps_menu_close`.
Closing through the API releases the mode; normal host input redraws the prompt.

![Numbered menu](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu.png)

![Centered single column](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-centered.png)

![Number column and details pane](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-details.png)
