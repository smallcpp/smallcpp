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

static void shade_layer(
        GtkTextView *view,
        GtkTextViewLayer layer,
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

    if (layer != GTK_TEXT_VIEW_LAYER_BELOW_TEXT)
        return;
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
