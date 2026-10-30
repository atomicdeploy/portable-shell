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
current item. Descriptions are clipped to the panel width. Numbering remains stable
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

![Number column and details panel](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-details.png)

## Filterable list mode

Run menu --list or call ps_menu_set_style(session, PS_MENU_LIST) before opening
a menu. The same menu items, submenu tree, disabled flags, action callbacks and
description callback are used; no separate item model is required.

The borderless list shows a title, item count, two-line items, a selected-item
rail, numeric/dot pagination and optional expanded keyboard help. Descriptions
come from ps_menu_set_description. Empty results and unavailable selections are
handled without dispatching an action.

- ↑/↓ or j/k select; ←/→ or Page Up/Page Down change pages.
- Home/End selects the first/last match; Enter opens the selected item.
- / edits the filter; Enter accepts it. Esc clears an active filter before
  returning to the parent menu. Ctrl-C closes the entire menu.
- ? toggles extended help. Clicking selects an item; wheel input moves selection.
- Filter editing supports ←/→, Home/End, Backspace, Delete and Ctrl-U. It uses the
  existing byte editor and bounded storage.

Filtering is stable-order subsequence matching on titles and descriptions, with
ASCII case folding and exact UTF-8 byte matching. It does not rank matches or
implement locale-aware Unicode case folding. Refresh/resizing preserves the filter.
Tiny terminals and non-interactive sessions use the existing numbered fallback.

PS_MENU_LIST_VIEW=0 removes the list renderer and its filter state. Numbered and
framed menus remain available. Existing defaults are unchanged.

This is an original C implementation inspired by the interaction pattern documented
by [Charm's list component](https://github.com/charmbracelet/bubbles#list), with no
Go code or dependencies imported. It does not yet include a list activity spinner
or automatic match highlighting.

![List and pagination, actual byte-output replay](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-list.png)

![Filtering, actual byte-output replay](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-filtered.png)
