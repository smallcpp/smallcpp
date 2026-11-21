/* Draw the current line under the text. A paragraph tag
 * cannot paint an empty line, and a range that reaches the
 * next line paints that line too. */
#include <gtk/gtk.h>

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

static void shade_layer(
        GtkTextView *view,
        GtkTextViewLayer layer,
        GtkSnapshot *snapshot) {
    if (layer == GTK_TEXT_VIEW_LAYER_BELOW_TEXT)
        draw_shade(view, snapshot);
    if (layer == GTK_TEXT_VIEW_LAYER_BELOW_TEXT
            || layer == GTK_TEXT_VIEW_LAYER_ABOVE_TEXT)
        draw_marks(view, layer, snapshot);
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
