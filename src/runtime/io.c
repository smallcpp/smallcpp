/*
 * print and format. Replacement fields are the ones the compiler accepts.
 * A field with no type reads an int. {:s} reads a string.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MaxArgs = 32, ArgInt = 0, ArgUnsigned = 1, ArgDouble = 2, ArgString = 3, ArgPointer = 4 };

struct Slot {
    int kind;
    long long i;
    unsigned long long u;
    double d;
    const char *s;
    void *p;
};

struct Rep {
    int index;
    int kind;
    char type;
};

struct Buf {
    char *data;
    size_t length;
    size_t cap;
};

static int append(struct Buf *buf, const char *text, size_t n)
{
    char *grown;

    if (buf->length + n + 1 > buf->cap) {
        size_t cap = buf->cap == 0 ? 64 : buf->cap;
        while (buf->length + n + 1 > cap)
            cap *= 2;
        grown = realloc(buf->data, cap);
        if (grown == NULL)
            return 0;
        buf->data = grown;
        buf->cap = cap;
    }
    memcpy(buf->data + buf->length, text, n);
    buf->length += n;
    buf->data[buf->length] = '\0';
    return 1;
}

static int append_str(struct Buf *buf, const char *text)
{
    if (text == NULL)
        text = "";
    return append(buf, text, strlen(text));
}

static int kind_of(char type)
{
    if (type == 's')
        return ArgString;
    if (type == 'p')
        return ArgPointer;
    if (type == 'f' || type == 'F' || type == 'e' || type == 'E' || type == 'g' || type == 'G'
        || type == 'a' || type == 'A')
        return ArgDouble;
    if (type == 'x' || type == 'X' || type == 'o' || type == 'u')
        return ArgUnsigned;
    return ArgInt;
}

static int read_reps(const char *text, struct Rep *reps, int *count, int kinds[MaxArgs], int *max_index)
{
    int auto_index = 0;
    int n = 0;
    const char *p = text;

    *max_index = -1;
    for (int i = 0; i < MaxArgs; i++)
        kinds[i] = -1;
    while (*p != '\0') {
        int index;
        char type = 0;

        if (*p == '{' && p[1] == '{') {
            p += 2;
            continue;
        }
        if (*p == '}' && p[1] == '}') {
            p += 2;
            continue;
        }
        if (*p != '{') {
            p++;
            continue;
        }
        p++;
        if (*p >= '0' && *p <= '9') {
            index = 0;
            while (*p >= '0' && *p <= '9') {
                index = index * 10 + (*p - '0');
                p++;
            }
        } else {
            index = auto_index++;
        }
        if (*p == ':') {
            p++;
            while (*p != '\0' && *p != '}') {
                if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))
                    type = *p;
                p++;
            }
        }
        if (*p != '}')
            return 0;
        p++;
        if (n >= MaxArgs || index < 0 || index >= MaxArgs)
            return 0;
        reps[n].index = index;
        reps[n].kind = kind_of(type);
        reps[n].type = type;
        n++;
        if (kinds[index] < 0)
            kinds[index] = reps[n - 1].kind;
        if (index > *max_index)
            *max_index = index;
    }
    *count = n;
    return 1;
}

static void load_args(struct Slot *slots, int kinds[MaxArgs], int max_index, va_list ap)
{
    int i;

    for (i = 0; i <= max_index; i++) {
        int kind = kinds[i] < 0 ? ArgInt : kinds[i];
        slots[i].kind = kind;
        if (kind == ArgString)
            slots[i].s = va_arg(ap, const char *);
        else if (kind == ArgPointer)
            slots[i].p = va_arg(ap, void *);
        else if (kind == ArgDouble)
            slots[i].d = va_arg(ap, double);
        else if (kind == ArgUnsigned)
            slots[i].u = va_arg(ap, unsigned);
        else
            slots[i].i = va_arg(ap, int);
    }
}

static int write_slot(struct Buf *buf, const struct Slot *slot, char type)
{
    char tmp[128];
    int n = 0;

    if (slot->kind == ArgString)
        return append_str(buf, slot->s);
    if (slot->kind == ArgPointer) {
        n = snprintf(tmp, sizeof tmp, "%p", slot->p);
    } else if (slot->kind == ArgDouble) {
        n = snprintf(tmp, sizeof tmp, "%g", slot->d);
    } else if (slot->kind == ArgUnsigned) {
        if (type == 'x')
            n = snprintf(tmp, sizeof tmp, "%x", (unsigned)slot->u);
        else if (type == 'X')
            n = snprintf(tmp, sizeof tmp, "%X", (unsigned)slot->u);
        else if (type == 'o')
            n = snprintf(tmp, sizeof tmp, "%o", (unsigned)slot->u);
        else
            n = snprintf(tmp, sizeof tmp, "%u", (unsigned)slot->u);
    } else if (type == 'c') {
        tmp[0] = (char)slot->i;
        tmp[1] = '\0';
        n = 1;
    } else {
        n = snprintf(tmp, sizeof tmp, "%d", (int)slot->i);
    }
    if (n < 0)
        return 0;
    return append(buf, tmp, (size_t)n);
}

static char *render(const char *text, va_list ap)
{
    struct Rep reps[MaxArgs];
    struct Slot slots[MaxArgs];
    int kinds[MaxArgs];
    int count = 0;
    int max_index = -1;
    int rep = 0;
    struct Buf buf;
    const char *p;

    memset(&buf, 0, sizeof buf);
    if (text == NULL)
        text = "";
    if (!read_reps(text, reps, &count, kinds, &max_index)) {
        free(buf.data);
        return NULL;
    }
    if (max_index >= 0)
        load_args(slots, kinds, max_index, ap);
    p = text;
    while (*p != '\0') {
        if (*p == '{' && p[1] == '{') {
            if (!append(&buf, "{", 1))
                break;
            p += 2;
            continue;
        }
        if (*p == '}' && p[1] == '}') {
            if (!append(&buf, "}", 1))
                break;
            p += 2;
            continue;
        }
        if (*p != '{') {
            const char *start = p;
            while (*p != '\0' && !(*p == '{' || (*p == '}' && p[1] == '}')))
                p++;
            if (!append(&buf, start, (size_t)(p - start)))
                break;
            continue;
        }
        while (*p != '\0' && *p != '}')
            p++;
        if (*p == '}')
            p++;
        if (rep < count) {
            if (!write_slot(&buf, &slots[reps[rep].index], reps[rep].type))
                break;
            rep++;
        }
    }
    if (buf.data == NULL)
        return calloc(1, 1);
    return buf.data;
}

void smallcpp_print(const char *text, ...)
{
    va_list ap;
    char *out;

    va_start(ap, text);
    out = render(text, ap);
    va_end(ap);
    if (out == NULL)
        return;
    fputs(out, stdout);
    fflush(stdout);
    free(out);
}

char *smallcpp_format(const char *text, ...)
{
    va_list ap;
    char *out;

    va_start(ap, text);
    out = render(text, ap);
    va_end(ap);
    if (out == NULL)
        out = calloc(1, 1);
    return out;
}
