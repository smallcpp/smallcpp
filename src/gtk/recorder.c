/* Record keystrokes on a text view and play them back.
 * GTK 4 has no key-event constructor, so playback performs
 * the key: insert text, delete, or move the cursor.
 * A key with Control or Alt is left for the caller. */
#include <gtk/gtk.h>
#include <string.h>

#define MACRO_KEYS 64
#define MACRO_NAME 64

typedef struct Recorder {
    GtkWidget *view;
    int count;
    int on;
    int playing;
    char keys[MACRO_KEYS][MACRO_NAME];
} Recorder;

static const char macro_empty[] = "";

static void macro_forget(gpointer data, GObject *dead) {
    Recorder *rec;

    (void)dead;
    rec = (Recorder *)data;
    if (rec == NULL)
        return;
    rec->view = NULL;
    rec->on = 0;
}

static Recorder *macro_of(GtkWidget *view) {
    Recorder *rec;

    rec = g_object_get_data(G_OBJECT(view), "smallcpp-macro");
    if (rec != NULL)
        return rec;
    rec = g_new0(Recorder, 1);
    rec->view = view;
    g_object_set_data(G_OBJECT(view), "smallcpp-macro", rec);
    g_object_weak_ref(G_OBJECT(view), macro_forget, rec);
    return rec;
}

static void macro_add(Recorder *rec, const char *name) {
    int n;
    int i;

    if (rec == NULL || name == NULL || name[0] == 0)
        return;
    if (rec->count >= MACRO_KEYS)
        return;
    n = 0;
    while (name[n] != 0)
        n = n + 1;
    if (n + 1 > MACRO_NAME)
        return;
    i = 0;
    while (i < n) {
        rec->keys[rec->count][i] = name[i];
        i = i + 1;
    }
    rec->keys[rec->count][n] = 0;
    rec->count = rec->count + 1;
}

static gboolean macro_key(
        GtkEventControllerKey *controller,
        guint keyval,
        guint keycode,
        GdkModifierType state,
        gpointer data) {
    Recorder *rec;
    guint mods;
    char *name;

    (void)controller;
    (void)keycode;
    rec = (Recorder *)data;
    if (rec == NULL || rec->view == NULL)
        return FALSE;
    if (rec->on == 0 || rec->playing != 0)
        return FALSE;
    mods = state & gtk_accelerator_get_default_mod_mask();
    if (gtk_accelerator_valid(keyval, mods) == FALSE)
        return FALSE;
    name = gtk_accelerator_name(keyval, mods);
    if (name == NULL)
        return FALSE;
    macro_add(rec, name);
    g_free(name);
    return FALSE;
}

static void macro_listen(GtkWidget *view, Recorder *rec) {
    GtkEventController *keys;

    if (g_object_get_data(G_OBJECT(view), "smallcpp-macro-keys")
            != NULL)
        return;
    keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(
            keys,
            GTK_PHASE_CAPTURE);
    gtk_widget_add_controller(view, keys);
    g_signal_connect(
            keys,
            "key-pressed",
            G_CALLBACK(macro_key),
            rec);
    g_object_set_data(G_OBJECT(view), "smallcpp-macro-keys", keys);
}

static void move_iter(GtkTextIter *iter, guint key) {
    if (key == GDK_KEY_Left)
        gtk_text_iter_backward_char(iter);
    else if (key == GDK_KEY_Right)
        gtk_text_iter_forward_char(iter);
    else if (key == GDK_KEY_Up)
        gtk_text_iter_backward_line(iter);
    else if (key == GDK_KEY_Down)
        gtk_text_iter_forward_line(iter);
    else if (key == GDK_KEY_Home)
        gtk_text_iter_set_line_offset(iter, 0);
    else if (key == GDK_KEY_End)
        gtk_text_iter_forward_to_line_end(iter);
}

static int play_motion(
        GtkTextBuffer *buffer,
        guint key,
        guint mods) {
    GtkTextIter insert;
    GtkTextIter bound;
    int extend;

    extend = 0;
    if ((mods & GDK_SHIFT_MASK) != 0)
        extend = 1;
    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &insert,
            gtk_text_buffer_get_insert(buffer));
    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &bound,
            gtk_text_buffer_get_selection_bound(buffer));
    move_iter(&insert, key);
    if (extend != 0)
        gtk_text_buffer_select_range(buffer, &insert, &bound);
    else
        gtk_text_buffer_place_cursor(buffer, &insert);
    return 1;
}

static int play_delete(GtkTextBuffer *buffer, int forward) {
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_iter_at_mark(
            buffer,
            &start,
            gtk_text_buffer_get_insert(buffer));
    end = start;
    if (forward == 0) {
        if (gtk_text_iter_backward_char(&start) == FALSE)
            return 1;
    }
    else if (gtk_text_iter_forward_char(&end) == FALSE)
        return 1;
    gtk_text_buffer_delete(buffer, &start, &end);
    return 1;
}

static int play_text(GtkTextBuffer *buffer, const char *text) {
    if (text == NULL || text[0] == 0)
        return 0;
    gtk_text_buffer_insert_at_cursor(buffer, text, -1);
    return 1;
}

void smallcpp_text_record(void *view, int on) {
    Recorder *rec;

    if (view == NULL)
        return;
    rec = macro_of(GTK_WIDGET(view));
    macro_listen(GTK_WIDGET(view), rec);
    if (on == 0) {
        rec->on = 0;
        return;
    }
    rec->count = 0;
    rec->on = 1;
}

int smallcpp_text_recording(void *view) {
    Recorder *rec;

    if (view == NULL)
        return 0;
    rec = g_object_get_data(G_OBJECT(view), "smallcpp-macro");
    if (rec == NULL || rec->on == 0)
        return 0;
    return 1;
}

int smallcpp_text_recorded(void *view) {
    Recorder *rec;

    if (view == NULL)
        return 0;
    rec = g_object_get_data(G_OBJECT(view), "smallcpp-macro");
    if (rec == NULL)
        return 0;
    return rec->count;
}

const char *smallcpp_text_recorded_key(void *view, int index) {
    Recorder *rec;

    if (view == NULL || index < 0)
        return macro_empty;
    rec = g_object_get_data(G_OBJECT(view), "smallcpp-macro");
    if (rec == NULL || index >= rec->count)
        return macro_empty;
    return rec->keys[index];
}

void smallcpp_text_record_key(void *view, const char *accel) {
    Recorder *rec;

    if (view == NULL)
        return;
    rec = g_object_get_data(G_OBJECT(view), "smallcpp-macro");
    if (rec == NULL || rec->on == 0 || rec->playing != 0)
        return;
    macro_add(rec, accel);
}

int smallcpp_text_play_key(void *view, const char *accel) {
    Recorder *rec;
    GtkTextBuffer *buffer;
    guint key;
    guint mods;
    guint blocked;
    gunichar ch;
    char utf8[8];
    int n;
    int handled;

    if (view == NULL || accel == NULL || accel[0] == 0)
        return 0;
    key = 0;
    mods = 0;
    if (gtk_accelerator_parse(accel, &key, &mods) == FALSE)
        return 0;
    if (key == 0)
        return 0;
    mods = mods & gtk_accelerator_get_default_mod_mask();
    blocked = GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK;
    if ((mods & blocked) != 0)
        return 0;
    rec = g_object_get_data(G_OBJECT(view), "smallcpp-macro");
    if (rec != NULL)
        rec->playing = 1;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    handled = 0;
    if (key == GDK_KEY_Left || key == GDK_KEY_Right
            || key == GDK_KEY_Up || key == GDK_KEY_Down
            || key == GDK_KEY_Home || key == GDK_KEY_End)
        handled = play_motion(buffer, key, mods);
    else if (key == GDK_KEY_BackSpace)
        handled = play_delete(buffer, 0);
    else if (key == GDK_KEY_Delete
            || key == GDK_KEY_KP_Delete)
        handled = play_delete(buffer, 1);
    else if (key == GDK_KEY_Return || key == GDK_KEY_KP_Enter)
        handled = play_text(buffer, "\n");
    else if (key == GDK_KEY_Tab)
        handled = play_text(buffer, "\t");
    else {
        ch = gdk_keyval_to_unicode(key);
        if ((mods & GDK_SHIFT_MASK) != 0)
            ch = g_unichar_toupper(ch);
        if (ch != 0) {
            n = g_unichar_to_utf8(ch, utf8);
            if (n > 0 && n < 8) {
                utf8[n] = 0;
                handled = play_text(buffer, utf8);
            }
        }
    }
    if (rec != NULL)
        rec->playing = 0;
    return handled;
}
