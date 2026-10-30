# 🔢 Numbered menus


`menu` (or host option `--menu`) opens a numbered list, with no table, alternate
screen, or mouse capture. Enter submits a number; `0` and Esc go back; Ctrl-C closes
the whole menu. Backspace edits the number. A leaf exits menu input mode before
calling its action, so actions may safely execute commands or open another view.
Disabled entries never invoke callbacks. Paste cannot activate menu actions.

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
