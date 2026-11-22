/* Draw the current line under the text. A paragraph tag
 * cannot paint an empty line, and a range that reaches the
 * next line paints that line too. */
#include <gtk/gtk.h>
#include <string.h>

typedef struct LineShade {
    int on;
    float red;
    float green;
    float blue;
    float alpha;
} LineShade;

static int hex_value(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

static void parse_color(const char *text, LineShade *shade) {
    int i;
    int n;
    int value;

    shade->red = 214.0f / 255.0f;
    shade->green = 228.0f / 255.0f;
    shade->blue = 245.0f / 255.0f;
    shade->alpha = 1.0f;
    if (text == NULL || text[0] != '#')
        return;
    n = 0;
    while (text[n] != 0)
        n = n + 1;
    if (n != 7)
        return;
    i = 0;
    while (i < 3) {
        int hi;
        int lo;

        hi = hex_value(text[1 + i * 2]);
        lo = hex_value(text[2 + i * 2]);
        if (hi < 0 || lo < 0)
            return;
        value = hi * 16 + lo;
        if (i == 0)
            shade->red = (float)value / 255.0f;
        if (i == 1)
            shade->green = (float)value / 255.0f;
        if (i == 2)
            shade->blue = (float)value / 255.0f;
        i = i + 1;
    }
}

typedef struct ViewMarks {
    int spaces;
    int ends;
    int stops;
    int tabWidth;
} ViewMarks;

static void paint_rect(
        GtkSnapshot *snapshot,
        float x,
        float y,
        float w,
        float h,
        float red,
        float green,
        float blue) {
    GdkRGBA color;
    graphene_rect_t rect;

    if (w < 1.0f || h < 1.0f)
        return;
    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = 1.0f;
    rect.origin.x = x;
    rect.origin.y = y;
    rect.size.width = w;
    rect.size.height = h;
    gtk_snapshot_append_color(snapshot, &color, &rect);
}

static int cell_width(GtkTextView *view) {
    PangoLayout *layout;
    int width;
    int height;

    layout = gtk_widget_create_pango_layout(
            GTK_WIDGET(view),
            "0000000000");
    if (layout == NULL)
        return 8;
    width = 0;
    height = 0;
    pango_layout_get_pixel_size(layout, &width, &height);
    g_object_unref(layout);
    if (width < 10)
        return 8;
    return width / 10;
}

static void draw_line_marks(
        GtkTextView *view,
        GtkSnapshot *snapshot,
        GtkTextIter *iter,
        ViewMarks *marks,
        int cell,
        int layer) {
    GtkTextIter cursor;
    GdkRectangle loc;
    int y;
    int height;
    int x0;
    int lead;
    int col;
    int inLead;
    int tab;

    y = 0;
    height = 0;
    gtk_text_view_get_line_yrange(view, iter, &y, &height);
    cursor = *iter;
    gtk_text_view_get_iter_location(view, &cursor, &loc);
    x0 = loc.x;
    lead = 0;
    col = 0;
    inLead = 1;
    tab = marks->tabWidth;
    if (tab < 1)
        tab = 4;
    while (gtk_text_iter_ends_line(&cursor) == 0) {
        unsigned int ch;
        int step;

        ch = gtk_text_iter_get_char(&cursor);
        gtk_text_view_get_iter_location(view, &cursor, &loc);
        step = 1;
        if (ch == '\t') {
            step = tab - (col % tab);
            if (step < 1)
                step = tab;
        }
        if (inLead != 0 && (ch == ' ' || ch == '\t'))
            lead = lead + step;
        else
            inLead = 0;
        if (layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT
                && marks->spaces != 0
                && ch == ' ')
            paint_rect(
                    snapshot,
                    (float)(loc.x + loc.width / 2 - 1),
                    (float)(loc.y + loc.height / 2 - 1),
                    2,
                    2,
                    0.45f,
                    0.45f,
                    0.45f);
        if (layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT
                && marks->spaces != 0
                && ch == '\t'
                && loc.width > 8)
            paint_rect(
                    snapshot,
                    (float)(loc.x + 2),
                    (float)(loc.y + loc.height / 2),
                    (float)(loc.width - 6),
                    1,
                    0.45f,
                    0.45f,
                    0.45f);
        col = col + step;
        if (col > 500)
            break;
        if (gtk_text_iter_forward_char(&cursor) == 0)
            break;
    }
    if (layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT
            && marks->ends != 0) {
        gtk_text_view_get_iter_location(view, &cursor, &loc);
        paint_rect(
                snapshot,
                (float)loc.x,
                (float)(loc.y + loc.height / 2),
                7,
                1,
                0.35f,
                0.35f,
                0.35f);
        paint_rect(
                snapshot,
                (float)(loc.x + 6),
                (float)(loc.y + loc.height / 2 - 5),
                1,
                5,
                0.35f,
                0.35f,
                0.35f);
    }
    if (layer == GTK_TEXT_VIEW_LAYER_BELOW_TEXT
            && marks->stops != 0
            && cell > 0) {
        int at;

        at = tab;
        while (at < lead && at < 400) {
            paint_rect(
                    snapshot,
                    (float)(x0 + at * cell),
                    (float)y,
                    1,
                    (float)height,
                    0.72f,
                    0.72f,
                    0.72f);
            at = at + tab;
        }
    }
}

static void draw_marks(
        GtkTextView *view,
        GtkTextViewLayer layer,
        GtkSnapshot *snapshot) {
    ViewMarks *marks;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    GdkRectangle visible;
    int cell;
    int lines;
    int y;
    int height;

    marks = g_object_get_data(G_OBJECT(view), "smallcpp-marks");
    if (marks == NULL)
        return;
    if (marks->spaces == 0 && marks->ends == 0 && marks->stops == 0)
        return;
    if (layer == GTK_TEXT_VIEW_LAYER_BELOW_TEXT && marks->stops == 0)
        return;
    if (layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT
            && marks->spaces == 0
            && marks->ends == 0)
        return;
    buffer = gtk_text_view_get_buffer(view);
    if (buffer == NULL)
        return;
    gtk_text_view_get_visible_rect(view, &visible);
    if (gtk_text_view_get_iter_at_position(
                view,
                &iter,
                NULL,
                visible.x,
                visible.y) == 0)
        gtk_text_buffer_get_start_iter(buffer, &iter);
    gtk_text_iter_set_line_offset(&iter, 0);
    cell = cell_width(view);
    lines = 0;
    while (lines < 80) {
        y = 0;
        height = 0;
        gtk_text_view_get_line_yrange(view, &iter, &y, &height);
        if (y > visible.y + visible.height)
            break;
        draw_line_marks(
                view,
                snapshot,
                &iter,
                marks,
                cell,
                (int)layer);
        lines = lines + 1;
        if (gtk_text_iter_forward_line(&iter) == 0)
            break;
    }
}

static void draw_shade(
        GtkTextView *view,
        GtkSnapshot *snapshot) {
    LineShade *shade;
    GtkTextBuffer *buffer;
    GtkTextMark *mark;
    GtkTextIter iter;
    GdkRectangle visible;
    int top;
    int height;
    GdkRGBA color;
    graphene_rect_t rect;

    shade = g_object_get_data(G_OBJECT(view), "smallcpp-line");
    if (shade == NULL || shade->on == 0)
        return;
    buffer = gtk_text_view_get_buffer(view);
    if (buffer == NULL)
        return;
    mark = gtk_text_buffer_get_insert(buffer);
    if (mark == NULL)
        return;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    top = 0;
    height = 0;
    gtk_text_view_get_line_yrange(view, &iter, &top, &height);
    if (height < 1)
        height = 1;
    gtk_text_view_get_visible_rect(view, &visible);
    color.red = shade->red;
    color.green = shade->green;
    color.blue = shade->blue;
    color.alpha = shade->alpha;
    rect.origin.x = 0;
    rect.origin.y = (float)top;
    rect.size.width = (float)(visible.x + visible.width);
    if (rect.size.width < 64.0f)
        rect.size.width = 10000.0f;
    rect.size.height = (float)height;
    gtk_snapshot_append_color(snapshot, &color, &rect);
}

static void (*saved_layer)(
        GtkTextView *view,
        GtkTextViewLayer layer,
        GtkSnapshot *snapshot);

static void draw_carets(
        GtkTextView *view,
        GtkSnapshot *snapshot);

static void shade_layer(
        GtkTextView *view,
        GtkTextViewLayer layer,
        GtkSnapshot *snapshot) {
    if (saved_layer != NULL && saved_layer != shade_layer)
        saved_layer(view, layer, snapshot);
    if (layer == GTK_TEXT_VIEW_LAYER_BELOW_TEXT)
        draw_shade(view, snapshot);
    if (layer == GTK_TEXT_VIEW_LAYER_BELOW_TEXT
            || layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT)
        draw_marks(view, layer, snapshot);
    if (layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT)
        draw_carets(view, snapshot);
}

static void install_shade(void) {
    GtkTextViewClass *klass;
    static int done = 0;

    if (done != 0)
        return;
    done = 1;
    klass = (GtkTextViewClass *)g_type_class_ref(
            gtk_text_view_get_type());
    if (klass == NULL)
        return;
    saved_layer = klass->snapshot_layer;
    klass->snapshot_layer = shade_layer;
}

void smallcpp_text_line_shade(
        void *view,
        int on,
        const char *color) {
    LineShade *shade;

    if (view == NULL)
        return;
    install_shade();
    shade = g_object_get_data(G_OBJECT(view), "smallcpp-line");
    if (shade == NULL) {
        shade = g_new0(LineShade, 1);
        g_object_set_data_full(
                G_OBJECT(view),
                "smallcpp-line",
                shade,
                g_free);
    }
    shade->on = on == 0 ? 0 : 1;
    parse_color(color, shade);
    gtk_widget_queue_draw(GTK_WIDGET(view));
}

void smallcpp_text_marks(
        void *view,
        int spaces,
        int ends,
        int stops,
        int tabWidth) {
    ViewMarks *marks;

    if (view == NULL)
        return;
    install_shade();
    marks = g_object_get_data(G_OBJECT(view), "smallcpp-marks");
    if (marks == NULL) {
        marks = g_new0(ViewMarks, 1);
        g_object_set_data_full(
                G_OBJECT(view),
                "smallcpp-marks",
                marks,
                g_free);
    }
    marks->spaces = spaces == 0 ? 0 : 1;
    marks->ends = ends == 0 ? 0 : 1;
    marks->stops = stops == 0 ? 0 : 1;
    if (tabWidth < 1)
        tabWidth = 4;
    marks->tabWidth = tabWidth;
    gtk_widget_queue_draw(GTK_WIDGET(view));
}

#define TEXT_CARETS 16

typedef struct CaretHook {
    GtkTextMark *extra[TEXT_CARETS];
    GtkTextBuffer *buffer;
    char *saved;
    int savedLen;
    int count;
    int busy;
    int hold;
    int hooked;
    int pending;
    int insertHere;
    int wave;
    int seen;
    int defer;
} CaretHook;

static CaretHook *carets_of(GtkTextView *view) {
    CaretHook *hook;

    hook = g_object_get_data(G_OBJECT(view), "smallcpp-carets");
    if (hook == NULL) {
        hook = g_new0(CaretHook, 1);
        g_object_set_data_full(
                G_OBJECT(view),
                "smallcpp-carets",
                hook,
                g_free);
    }
    return hook;
}

static void block_changed(GtkTextBuffer *buffer) {
    g_signal_handlers_block_matched(
            buffer,
            G_SIGNAL_MATCH_ID,
            g_signal_lookup("changed", GTK_TYPE_TEXT_BUFFER),
            0,
            NULL,
            NULL,
            NULL);
}

static void unblock_changed(GtkTextBuffer *buffer) {
    g_signal_handlers_unblock_matched(
            buffer,
            G_SIGNAL_MATCH_ID,
            g_signal_lookup("changed", GTK_TYPE_TEXT_BUFFER),
            0,
            NULL,
            NULL,
            NULL);
}

static void prune_carets(GtkTextView *view, CaretHook *hook) {
    GtkTextBuffer *buffer;
    GtkTextIter cursor;
    int i;

    buffer = gtk_text_view_get_buffer(view);
    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &cursor,
            gtk_text_buffer_get_insert(buffer));
    i = 0;
    while (i < hook->count) {
        GtkTextIter at;
        int drop;
        int j;

        drop = 0;
        if (hook->extra[i] == NULL)
            drop = 1;
        else {
            gtk_text_buffer_get_iter_at_mark(
                    buffer,
                    &at,
                    hook->extra[i]);
            if (gtk_text_iter_equal(&at, &cursor))
                drop = 1;
            j = 0;
            while (j < i && drop == 0) {
                GtkTextIter other;

                if (hook->extra[j] != NULL) {
                    gtk_text_buffer_get_iter_at_mark(
                            buffer,
                            &other,
                            hook->extra[j]);
                    if (gtk_text_iter_equal(&at, &other))
                        drop = 1;
                }
                j = j + 1;
            }
        }
        if (drop != 0) {
            if (hook->extra[i] != NULL)
                gtk_text_buffer_delete_mark(
                        buffer,
                        hook->extra[i]);
            j = i;
            while (j + 1 < hook->count) {
                hook->extra[j] = hook->extra[j + 1];
                j = j + 1;
            }
            hook->count = hook->count - 1;
        }
        else
            i = i + 1;
    }
}

static void caret_insert_before(
        GtkTextBuffer *buffer,
        GtkTextIter *location,
        const char *text,
        int len,
        gpointer data) {
    CaretHook *hook;
    GtkTextIter cursor;
    int cur;
    int at;

    hook = carets_of(GTK_TEXT_VIEW(data));
    if (hook->busy != 0 || hook->hold != 0 || hook->count == 0)
        return;
    if (text == NULL || len == 0)
        return;
    if (len < 0)
        len = (int)strlen(text);
    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &cursor,
            gtk_text_buffer_get_insert(buffer));
    cur = gtk_text_iter_get_offset(&cursor);
    at = gtk_text_iter_get_offset(location);
    if (at != cur)
        return;
    if (hook->saved != NULL)
        g_free(hook->saved);
    hook->saved = g_malloc((gsize)len + 1);
    if (hook->saved == NULL)
        return;
    memcpy(hook->saved, text, (size_t)len);
    hook->saved[len] = 0;
    hook->savedLen = len;
    hook->insertHere = 1;
    hook->wave = hook->wave + 1;
    hook->defer = 1;
}

static void caret_insert_after(
        GtkTextBuffer *buffer,
        GtkTextIter *location,
        const char *text,
        int len,
        gpointer data) {
    CaretHook *hook;
    GtkTextView *view;
    char *copy;
    int n;
    int i;

    (void)location;
    (void)text;
    (void)len;
    view = GTK_TEXT_VIEW(data);
    hook = carets_of(view);
    if (hook->insertHere == 0)
        return;
    hook->insertHere = 0;
    copy = hook->saved;
    n = hook->savedLen;
    hook->saved = NULL;
    hook->savedLen = 0;
    if (copy == NULL || n <= 0) {
        hook->defer = 0;
        return;
    }
    hook->busy = 1;
    block_changed(buffer);
    i = 0;
    while (i < hook->count) {
        GtkTextIter at;
        GtkTextIter cursor;

        if (hook->extra[i] != NULL) {
            gtk_text_buffer_get_iter_at_mark(
                    buffer,
                    &at,
                    hook->extra[i]);
            gtk_text_buffer_get_iter_at_mark(
                    buffer,
                    &cursor,
                    gtk_text_buffer_get_insert(buffer));
            if (gtk_text_iter_equal(&at, &cursor) == FALSE)
                gtk_text_buffer_insert(buffer, &at, copy, n);
        }
        i = i + 1;
    }
    unblock_changed(buffer);
    g_free(copy);
    prune_carets(view, hook);
    hook->busy = 0;
    hook->defer = 0;
    g_signal_emit_by_name(buffer, "changed");
    gtk_widget_queue_draw(GTK_WIDGET(view));
}

static void caret_delete_before(
        GtkTextBuffer *buffer,
        GtkTextIter *start,
        GtkTextIter *end,
        gpointer data) {
    CaretHook *hook;
    GtkTextIter cursor;
    int startOff;
    int endOff;
    int cur;

    hook = carets_of(GTK_TEXT_VIEW(data));
    if (hook->busy != 0 || hook->hold != 0 || hook->count == 0)
        return;
    hook->pending = 0;
    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &cursor,
            gtk_text_buffer_get_insert(buffer));
    startOff = gtk_text_iter_get_offset(start);
    endOff = gtk_text_iter_get_offset(end);
    cur = gtk_text_iter_get_offset(&cursor);
    if (endOff != startOff + 1)
        return;
    if (endOff == cur && startOff == cur - 1)
        hook->pending = 1;
    else if (startOff == cur && endOff == cur + 1)
        hook->pending = 2;
    if (hook->pending != 0) {
        hook->wave = hook->wave + 1;
        hook->defer = 1;
    }
}

static void caret_delete_after(
        GtkTextBuffer *buffer,
        GtkTextIter *start,
        GtkTextIter *end,
        gpointer data) {
    CaretHook *hook;
    GtkTextView *view;
    int dir;
    int i;

    (void)start;
    (void)end;
    view = GTK_TEXT_VIEW(data);
    hook = carets_of(view);
    dir = hook->pending;
    hook->pending = 0;
    if (dir == 0)
        return;
    hook->busy = 1;
    block_changed(buffer);
    i = hook->count - 1;
    while (i >= 0) {
        GtkTextIter at;
        GtkTextIter bound;

        if (hook->extra[i] != NULL) {
            gtk_text_buffer_get_iter_at_mark(
                    buffer,
                    &at,
                    hook->extra[i]);
            bound = at;
            if (dir == 1) {
                if (gtk_text_iter_backward_char(&at))
                    gtk_text_buffer_delete(buffer, &at, &bound);
            }
            else if (gtk_text_iter_forward_char(&bound))
                gtk_text_buffer_delete(buffer, &at, &bound);
        }
        i = i - 1;
    }
    unblock_changed(buffer);
    prune_carets(view, hook);
    hook->busy = 0;
    hook->defer = 0;
    g_signal_emit_by_name(buffer, "changed");
    gtk_widget_queue_draw(GTK_WIDGET(view));
}

static void ensure_hooks(GtkTextView *view, CaretHook *hook) {
    GtkTextBuffer *buffer;

    buffer = gtk_text_view_get_buffer(view);
    if (hook->hooked != 0 && hook->buffer == buffer)
        return;
    if (hook->hooked != 0 && hook->buffer != NULL)
        g_signal_handlers_disconnect_by_data(
                hook->buffer,
                view);
    hook->buffer = buffer;
    hook->hooked = 1;
    g_signal_connect(
            buffer,
            "insert-text",
            G_CALLBACK(caret_insert_before),
            view);
    g_signal_connect_after(
            buffer,
            "insert-text",
            G_CALLBACK(caret_insert_after),
            view);
    g_signal_connect(
            buffer,
            "delete-range",
            G_CALLBACK(caret_delete_before),
            view);
    g_signal_connect_after(
            buffer,
            "delete-range",
            G_CALLBACK(caret_delete_after),
            view);
}

static void draw_carets(
        GtkTextView *view,
        GtkSnapshot *snapshot) {
    CaretHook *hook;
    int i;

    hook = g_object_get_data(G_OBJECT(view), "smallcpp-carets");
    if (hook == NULL || hook->count == 0)
        return;
    i = 0;
    while (i < hook->count) {
        GtkTextIter at;
        GdkRectangle where;
        float tall;

        if (hook->extra[i] != NULL) {
            gtk_text_buffer_get_iter_at_mark(
                    hook->buffer,
                    &at,
                    hook->extra[i]);
            gtk_text_view_get_iter_location(view, &at, &where);
            tall = (float)where.height;
            if (tall < 1.0f)
                tall = 14.0f;
            paint_rect(
                    snapshot,
                    (float)where.x,
                    (float)where.y,
                    2.0f,
                    tall,
                    0.15f,
                    0.15f,
                    0.15f);
        }
        i = i + 1;
    }
}

int smallcpp_text_add_caret(
        void *view,
        long line,
        long offset) {
    GtkTextView *text;
    GtkTextBuffer *buffer;
    CaretHook *hook;
    GtkTextIter iter;
    GtkTextIter cursor;
    int i;

    if (view == NULL || line < 0 || offset < 0)
        return 0;
    text = GTK_TEXT_VIEW(view);
    install_shade();
    hook = carets_of(text);
    ensure_hooks(text, hook);
    if (hook->count >= TEXT_CARETS)
        return 0;
    buffer = gtk_text_view_get_buffer(text);
    if (gtk_text_buffer_get_iter_at_line_offset(
                buffer,
                &iter,
                (int)line,
                (int)offset) == FALSE)
        return 0;
    if (gtk_text_iter_get_line(&iter) != (int)line)
        return 0;
    if (gtk_text_iter_get_line_offset(&iter) != (int)offset)
        return 0;
    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &cursor,
            gtk_text_buffer_get_insert(buffer));
    if (gtk_text_iter_equal(&iter, &cursor))
        return 0;
    i = 0;
    while (i < hook->count) {
        GtkTextIter at;

        if (hook->extra[i] != NULL) {
            gtk_text_buffer_get_iter_at_mark(
                    buffer,
                    &at,
                    hook->extra[i]);
            if (gtk_text_iter_equal(&iter, &at))
                return 0;
        }
        i = i + 1;
    }
    hook->extra[hook->count] = gtk_text_buffer_create_mark(
            buffer,
            NULL,
            &iter,
            FALSE);
    if (hook->extra[hook->count] == NULL)
        return 0;
    hook->count = hook->count + 1;
    gtk_widget_queue_draw(GTK_WIDGET(text));
    return 1;
}

void smallcpp_text_clear_carets(void *view) {
    GtkTextView *text;
    GtkTextBuffer *buffer;
    CaretHook *hook;
    int i;

    if (view == NULL)
        return;
    text = GTK_TEXT_VIEW(view);
    hook = g_object_get_data(G_OBJECT(text), "smallcpp-carets");
    if (hook == NULL)
        return;
    buffer = gtk_text_view_get_buffer(text);
    i = 0;
    while (i < hook->count) {
        if (hook->extra[i] != NULL)
            gtk_text_buffer_delete_mark(buffer, hook->extra[i]);
        hook->extra[i] = NULL;
        i = i + 1;
    }
    hook->count = 0;
    gtk_widget_queue_draw(GTK_WIDGET(text));
}

int smallcpp_text_caret_count(void *view) {
    CaretHook *hook;

    if (view == NULL)
        return 0;
    hook = g_object_get_data(
            G_OBJECT(view),
            "smallcpp-carets");
    if (hook == NULL)
        return 1;
    return hook->count + 1;
}

int smallcpp_text_caret_at(
        void *view,
        int index,
        long *line,
        long *offset) {
    GtkTextView *text;
    GtkTextBuffer *buffer;
    CaretHook *hook;
    GtkTextIter at;

    if (view == NULL || index < 0)
        return 0;
    text = GTK_TEXT_VIEW(view);
    buffer = gtk_text_view_get_buffer(text);
    if (index == 0) {
        gtk_text_buffer_get_iter_at_mark(
                buffer,
                &at,
                gtk_text_buffer_get_insert(buffer));
    }
    else {
        hook = g_object_get_data(
                G_OBJECT(text),
                "smallcpp-carets");
        if (hook == NULL || index - 1 >= hook->count)
            return 0;
        if (hook->extra[index - 1] == NULL)
            return 0;
        gtk_text_buffer_get_iter_at_mark(
                buffer,
                &at,
                hook->extra[index - 1]);
    }
    if (line != NULL)
        *line = gtk_text_iter_get_line(&at);
    if (offset != NULL)
        *offset = gtk_text_iter_get_line_offset(&at);
    return 1;
}

int smallcpp_text_caret_deferred(void *view) {
    CaretHook *hook;

    if (view == NULL)
        return 0;
    hook = g_object_get_data(G_OBJECT(view), "smallcpp-carets");
    if (hook == NULL)
        return 0;
    return hook->defer;
}

int smallcpp_text_edit_motion(void *view) {
    CaretHook *hook;

    if (view == NULL)
        return 0;
    hook = g_object_get_data(G_OBJECT(view), "smallcpp-carets");
    if (hook == NULL)
        return 0;
    if (hook->wave != hook->seen)
        return 1;
    return 0;
}

void smallcpp_text_ack_edit(void *view) {
    CaretHook *hook;

    if (view == NULL)
        return;
    hook = g_object_get_data(G_OBJECT(view), "smallcpp-carets");
    if (hook == NULL)
        return;
    hook->seen = hook->wave;
}

void smallcpp_text_hold_carets(void *view, int hold) {
    GtkTextView *text;
    CaretHook *hook;

    if (view == NULL)
        return;
    text = GTK_TEXT_VIEW(view);
    hook = carets_of(text);
    ensure_hooks(text, hook);
    if (hold == 0)
        hook->hold = 0;
    else
        hook->hold = 1;
}
