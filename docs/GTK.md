# GTK

Include the header as `<gtk>`. The classes live in namespace `Gtk`. Link with `-l smallcpp_gtk -l gtk-4 -l gio-2.0 -l gobject-2.0 -l glib-2.0 -l pango-1.0 -l pangocairo-1.0 -l cairo`.

A null string throws `Exception` with message `null`. Invalid UTF-8 throws `utf8`. A call from a thread other than the GUI thread throws `thread`, except `Application::post`. Use after `dispose` throws `disposed`. A window or dialog used as a child, or a child that already has a parent, throws `parent`. A second child of a single-child container throws `child`. A bad index, orientation, or policy throws `range`. A second `Application` throws `gtk`. A second `run` throws `run`. `post` outside `run`, or past 64 queued posts, throws `post`. No display throws `display`.

`dispose` is explicit. The destructor does not dispose, and the collector does not tear a widget down. `GTK_HORIZONTAL` is 0 and `GTK_VERTICAL` is 1. `GTK_POLICY_ALWAYS` through `GTK_POLICY_EXTERNAL` are 0 through 3.

A running example catches `display`, prints `SKIP gtk display`, and returns 77 when GTK cannot open a display. CTest treats that as a skip. A call that blocks on a click, a file chooser, or the application loop is a separate file. That file compiles with `-c` and is not run.

## Widget

`Widget` is the base. `set_visible`, `set_sensitive`, `set_h_expand`, and `set_v_expand` set those properties. `set_width_request` sets the requested width. `add_css_class` and `remove_css_class` change the CSS classes. `set_tooltip` sets the tooltip. `disposed` is true after `dispose`. [`docs/examples/gtk-widget.scpp`](examples/gtk-widget.scpp) prints `gtk-widget 1`.

## Window

`Window` takes a title. `set_title` and `title` read it. `set_default_size` sets the size. `set_child` places one child. `remove` takes it back. `present` shows the window and returns. `close` closes it and returns. `on_close_request` runs when the window is asked to close. The callback returns true to keep the window. [`docs/examples/gtk-window.scpp`](examples/gtk-window.scpp) prints `gtk-window 1`.

## Box, label, and scrolled window

`Box` lays children in a row or a column, with a spacing in pixels. `append` adds a child. `remove` takes it back. [`docs/examples/gtk-box.scpp`](examples/gtk-box.scpp) prints `gtk-box 1`.

`Label` shows text. `set_text` and `text` read it. [`docs/examples/gtk-label.scpp`](examples/gtk-label.scpp) prints `gtk-label 1`.

`ScrolledWindow` scrolls one child. `set_policy` takes a horizontal policy and a vertical policy. `set_child` and `remove` manage that child. [`docs/examples/gtk-scrolled.scpp`](examples/gtk-scrolled.scpp) prints `gtk-scrolled 1`.

## Button, entry, and check

`Button` shows a label. `set_label` and `label` read it. `on_clicked` runs when the button is clicked. [`docs/examples/gtk-button.scpp`](examples/gtk-button.scpp) prints `gtk-button 1`.

`Entry` edits one line. `set_text` and `text` read it. `on_changed` runs when the text changes, including a call to `set_text`. [`docs/examples/gtk-entry.scpp`](examples/gtk-entry.scpp) prints `gtk-entry 1`.

`CheckButton` is a checkbox. `set_label` and `label` read the label. `set_active` and `active` read the check. `on_toggled` runs when the check changes. [`docs/examples/gtk-check.scpp`](examples/gtk-check.scpp) prints `gtk-check 1`.

## Header, menu, and popover

`HeaderBar` is a title bar. `set_title` and `title` read the title. `set_show_title_buttons` and `show_title_buttons` control the window buttons. `pack_start` and `pack_end` place children at the two ends. [`docs/examples/gtk-header.scpp`](examples/gtk-header.scpp) prints `gtk-header 1`.

`MenuButton` shows a label and a menu. `set_label` and `label` read the label. `set_menu` takes the labels, the count, and a callback of `(void *, const char *)`. [`docs/examples/gtk-menu.scpp`](examples/gtk-menu.scpp) prints `gtk-menu 1`. `MenuBar` is a bar of submenus. `add_menu` appends one submenu and returns its index. `set_menu` replaces the labels of that submenu. A count of zero leaves the submenu empty. The callback is the same `(void *, const char *)` and receives the original label.

`Popover` shows one child. `set_child` places it. [`docs/examples/gtk-popover.scpp`](examples/gtk-popover.scpp) prints `gtk-popover 1`. `popup` and `popdown` show and hide the popover. They need a toplevel, so those calls are compiled and not run. [`docs/examples/gtk-popover-popup.scpp`](examples/gtk-popover-popup.scpp) and [`docs/examples/gtk-popover-popdown.scpp`](examples/gtk-popover-popdown.scpp) compile with `-c`.

## Paned, notebook, and column view

`Paned` splits two children. `orientation` is `GTK_HORIZONTAL` or `GTK_VERTICAL`. `set_start_child` and `set_end_child` place them. `set_position` and `position` read the divider. [`docs/examples/gtk-paned.scpp`](examples/gtk-paned.scpp) prints `gtk-paned 1`.

`Notebook` holds pages. `append_page` adds a child and a title and returns the index. `set_current_page` and `current_page` read the visible page. `remove_page` drops one. `on_switch_page` runs when the page changes. [`docs/examples/gtk-notebook.scpp`](examples/gtk-notebook.scpp) prints `gtk-notebook 1`.

`ColumnView` stores text columns and rows. `append_column` adds a title. `append_row` adds one row of cell strings. `cell` reads a row and a column. `row_count` and `column_count` are the sizes. `clear_rows` drops the rows and keeps the columns. `on_activate` runs when a row is activated. [`docs/examples/gtk-column.scpp`](examples/gtk-column.scpp) prints `gtk-column 1`.

## Dialogs and files

`Dialog` takes a title. `set_title` and `title` read it. `set_content` places the body. `add_button` adds a button and the response integer it reports. `on_response` runs with that integer. `present` shows the dialog and returns. [`docs/examples/gtk-dialog.scpp`](examples/gtk-dialog.scpp) prints `gtk-dialog 1`.

`AlertDialog` takes a message and a detail. `message` and `detail` read them. [`docs/examples/gtk-alert.scpp`](examples/gtk-alert.scpp) prints `gtk-alert 1`. `present` waits for a confirmation, so that call is compiled and not run. [`docs/examples/gtk-alert-present.scpp`](examples/gtk-alert-present.scpp) compiles with `-c`.

`FileDialog` chooses a file. `set_title` and `title` read the title. [`docs/examples/gtk-file.scpp`](examples/gtk-file.scpp) prints `gtk-file 1`. `open` presents a multi-select chooser and calls back once for each selected path, in list order. Cancel, or a selection with no path, calls back once with null. `save` still calls back with one path. Those calls are compiled and not run. [`docs/examples/gtk-file-open.scpp`](examples/gtk-file-open.scpp) and [`docs/examples/gtk-file-save.scpp`](examples/gtk-file-save.scpp) compile with `-c`.

## Dropdown, spin, picture, progress, and spinner

`DropDown` lists strings. `set_items` replaces the items. `set_selected` and `selected` read the index. `selected_string` is the text. `on_changed` runs when the selection changes. [`docs/examples/gtk-dropdown.scpp`](examples/gtk-dropdown.scpp) prints `gtk-dropdown 1`.

`SpinButton` takes a minimum, a maximum, and a value. `min`, `max`, and `value` read them. `set_value` writes the value and runs `on_changed`. [`docs/examples/gtk-spin.scpp`](examples/gtk-spin.scpp) prints `gtk-spin 1`.

`Picture` shows a file. `set_file` and `file` read the path. [`docs/examples/gtk-picture.scpp`](examples/gtk-picture.scpp) prints `gtk-picture 1`.

`ProgressBar` shows a fraction from 0 to 1. `set_fraction` and `fraction` read it. [`docs/examples/gtk-progress.scpp`](examples/gtk-progress.scpp) prints `gtk-progress 1`.

`Spinner` shows activity. `start` and `stop` change it. `spinning` reports the state. [`docs/examples/gtk-spinner.scpp`](examples/gtk-spinner.scpp) prints `gtk-spinner 1`.

## Text editor

`TextIter` stores a line and a character offset. It is not a live iterator. Copy the two numbers to keep a position. `line` and `offset` read them.

`TextBuffer` holds the text. `set_text` replaces it. `get_text` returns it. `insert` writes text at an iterator. `delete_range` removes a half-open range. `place_cursor` moves the cursor. `select_range` selects a range. `selection_bounds` writes the selection into two iterators and returns whether a selection exists. `create_tag` names a tag and a foreground color. `apply_tag` and `remove_tag` change that tag on a range. `undo` and `redo` walk the edit history. `on_changed` runs when the text changes. `create_mark` returns a `TextMark` at a line and offset.

`TextMark` stays with the text. `name` is the name passed to `create_mark`. `move` sets a new line and offset. `line` and `offset` read them.

`TextView` displays a buffer. `set_editable` and `editable` control editing. `set_monospace` and `monospace` control the font. `set_font` takes a description such as `Monospace 12` and sets the family and the size on the view and on its line-number gutter. `scroll_to` scrolls to a line and offset, and it keeps that place on screen after the buffer changes. `set_line_numbers` shows a gutter and draws one number at each buffer line, so the numbers scroll with the text. The gutter is wide enough for four digits and one space before the text. An empty buffer still shows line 1. A line with no characters, including one Enter just inserted, still has a number. `set_line_shade` paints the cursor line under the text. `line_numbers` reads that switch. `line_number_text` is the gutter text. `line_number_origin` returns a line's y in the gutter, or -1 when that line is off screen. `set_column_marker` draws a vertical line two pixels wide at a character column. A column below 1 removes it. `on_move_cursor` runs when the cursor moves. `Widget::add_shortcut` binds an accelerator such as `<Control>s`. The callback is `void (*)(void *)`. The shortcut runs in the capture phase, so it is handled before the text view's own keys. `Widget::add_css` adds a CSS string to that widget.

`Dialog::set_transient` names the editor window before `present`, so GTK does not warn that the dialog has no parent. That present does not add the dialog to the application. `Window::destroy` destroys the window. `Application::quit` makes `run` return. `FileDialog::set_parent` is the window passed to the open and save calls. `FontDialog::choose` opens the font chooser for a parent window and calls back with the description. `Notebook::set_tab_label` changes a title. `set_tab` replaces the tab widget. `TextBuffer::set_tag_background` sets a paragraph background on a named tag.

[`docs/examples/gtk-text.scpp`](examples/gtk-text.scpp) builds a buffer, inserts and undoes, redoes, deletes a range, tags a selection, and reads the view. It prints `gtk-text 1`.

## Signals

`SignalConnection` is the handle returned by an `on_` method. `connected` is true while the handler is live. `disconnect` removes it. A fresh connection is not live, and `disconnect` on that connection returns. [`docs/examples/gtk-signal.scpp`](examples/gtk-signal.scpp) prints `gtk-signal 1`.

## Application

`Application` takes an application id such as `com.smallcpp.docs`. One process has one `Application`. `on_activate` runs when the application starts. `run` enters the loop and returns the status. A second `run` throws `run`. [`docs/examples/gtk-app-run.scpp`](examples/gtk-app-run.scpp) compiles with `-c` because `run` does not return until the application quits.

`post` queues a callback on the GUI thread and returns a token. Outside `run` it throws `post`. `cancel` drops a queued token and returns false when the token is not queued. [`docs/examples/gtk-app.scpp`](examples/gtk-app.scpp) prints `gtk-app 1`.
