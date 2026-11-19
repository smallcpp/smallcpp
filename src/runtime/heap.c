/*
 * Allocator declared for new. delete does not free.
 * An unreachable object is reclaimed at a later allocation. If its
 * destructor has not run, the collector calls it. The order of those
 * calls is not one a program can rely on.
 */
#include "internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { CollectFloor = 64 };

struct Object {
    uintptr_t base;
    uint64_t size;
    void (*dtor)(void *);
    int marked;
};

struct Dead {
    uintptr_t base;
    uint64_t size;
    void (*dtor)(void *);
};

struct Safepoint {
    void *result;
    uintptr_t *traced;
    int traced_count;
    int traced_cap;
};

static struct Object *objects;
static int object_count;
static int object_cap;
static int *by_base;
static int by_cap;
static int sorted_valid;
static uintptr_t heap_lo;
static uintptr_t heap_hi;
static uint64_t live_bytes;
static uint64_t fresh_bytes;
static uint64_t survived_bytes;
static struct Safepoint *points;
static int point_count;
static int point_cap;
static int collecting;
static int allocs;
static void *kept_result[2];
static uintptr_t *kept_roots;
static int kept_root_n;

static void mark_addr(uintptr_t addr);

static int cmp_base(const void *left, const void *right)
{
    uintptr_t a;
    uintptr_t b;

    a = objects[*(const int *)left].base;
    b = objects[*(const int *)right].base;
    if (a < b)
        return -1;
    if (a > b)
        return 1;
    return 0;
}

static void note_span(uintptr_t base, uint64_t size)
{
    uintptr_t end;

    end = base + (uintptr_t)size;
    if (heap_lo == 0 || base < heap_lo)
        heap_lo = base;
    if (end > heap_hi)
        heap_hi = end;
}

static void recompute_bounds(void)
{
    int i;

    heap_lo = 0;
    heap_hi = 0;
    for (i = 0; i < object_count; i++)
        note_span(objects[i].base, objects[i].size);
}

static void sort_spans(void)
{
    int i;
    int *grown;

    if (object_count > by_cap) {
        int cap;

        cap = object_cap > object_count ? object_cap : object_count;
        grown = realloc(by_base, (size_t)cap * sizeof *grown);
        if (grown == NULL)
            return;
        by_base = grown;
        by_cap = cap;
    }
    for (i = 0; i < object_count; i++)
        by_base[i] = i;
    if (object_count > 1)
        qsort(by_base, (size_t)object_count, sizeof *by_base, cmp_base);
    sorted_valid = 1;
}

static int lookup_base(uintptr_t addr, int exact)
{
    int lo;
    int hi;
    int best;

    if (object_count == 0)
        return -1;
    if (!sorted_valid)
        sort_spans();
    if (!sorted_valid) {
        int i;

        for (i = 0; i < object_count; i++) {
            if (exact) {
                if (objects[i].base == addr)
                    return i;
                continue;
            }
            if (addr >= objects[i].base && addr < objects[i].base + objects[i].size)
                return i;
        }
        return -1;
    }
    lo = 0;
    hi = object_count;
    best = -1;
    while (lo < hi) {
        int mid;
        uintptr_t base;

        mid = lo + (hi - lo) / 2;
        base = objects[by_base[mid]].base;
        if (base == addr)
            return by_base[mid];
        if (base < addr) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (exact || best < 0)
        return -1;
    best = by_base[best];
    if (addr >= objects[best].base && addr < objects[best].base + objects[best].size)
        return best;
    return -1;
}

static int pointer_word(uintptr_t addr)
{
    if (addr < 4096 || (addr & (sizeof(uintptr_t) - 1)) != 0)
        return 0;
    if (heap_lo == 0 || addr < heap_lo || addr >= heap_hi)
        return 0;
    return 1;
}

static void scan_object(int index)
{
    uintptr_t cursor;
    uintptr_t end;

    end = objects[index].base + objects[index].size;
    for (cursor = objects[index].base; cursor + sizeof(uintptr_t) <= end;
         cursor += sizeof(uintptr_t)) {
        uintptr_t word;

        memcpy(&word, (void *)cursor, sizeof word);
        if (!pointer_word(word))
            continue;
        mark_addr(word);
    }
}

static void mark_addr(uintptr_t addr)
{
    int index;

    if (!pointer_word(addr))
        return;
    index = lookup_base(addr, 0);
    if (index < 0 || objects[index].marked)
        return;
    objects[index].marked = 1;
    scan_object(index);
}

static int should_collect(void)
{
    if (object_count < CollectFloor)
        return 1;
    if (fresh_bytes == 0)
        return 0;
    if (survived_bytes == 0)
        return 1;
    return fresh_bytes >= survived_bytes;
}

static void reclaim(int index)
{
    struct Dead *dead;
    int dead_count = 0;
    int i;
    int kept;

    if (index < 0 || index >= point_count)
        return;
    sorted_valid = 0;
    recompute_bounds();
    for (i = 0; i < object_count; i++)
        objects[i].marked = 0;
    for (i = 0; i < points[index].traced_count; i++)
        mark_addr(points[index].traced[i]);
    for (i = 0; i < object_count; i++) {
        if (!objects[i].marked)
            dead_count++;
    }
    if (dead_count == 0) {
        fresh_bytes = 0;
        survived_bytes = live_bytes;
        return;
    }
    dead = malloc((size_t)dead_count * sizeof *dead);
    if (dead == NULL)
        return;
    dead_count = 0;
    kept = 0;
    for (i = 0; i < object_count; i++) {
        if (objects[i].marked) {
            objects[kept++] = objects[i];
            continue;
        }
        dead[dead_count].base = objects[i].base;
        dead[dead_count].size = objects[i].size;
        dead[dead_count].dtor = objects[i].dtor;
        dead_count++;
    }
    object_count = kept;
    sorted_valid = 0;
    for (i = 0; i < dead_count; i++) {
        if (live_bytes >= dead[i].size)
            live_bytes -= dead[i].size;
        else
            live_bytes = 0;
        if (dead[i].dtor != NULL)
            dead[i].dtor((void *)dead[i].base);
        free((void *)dead[i].base);
    }
    free(dead);
    fresh_bytes = 0;
    survived_bytes = live_bytes;
    sorted_valid = 0;
    recompute_bounds();
}

int smallcpp_heap_contains(uintptr_t addr)
{
    if (!pointer_word(addr))
        return 0;
    return lookup_base(addr, 0) >= 0;
}

void smallcpp_trace_pointer(int index, uintptr_t addr)
{
    struct Safepoint *point;
    int i;

    if (index < 0 || index >= point_count || !smallcpp_heap_contains(addr))
        return;
    point = &points[index];
    for (i = 0; i < point->traced_count; i++) {
        if (point->traced[i] == addr)
            return;
    }
    if (point->traced_count == point->traced_cap) {
        int cap = point->traced_cap == 0 ? 4 : point->traced_cap * 2;
        uintptr_t *grown = realloc(point->traced, (size_t)cap * sizeof *grown);
        if (grown == NULL)
            return;
        point->traced = grown;
        point->traced_cap = cap;
    }
    point->traced[point->traced_count++] = addr;
}

static void *heap_alloc(uint64_t bytes, uint64_t count)
{
    uint64_t nbytes;
    void *ptr;
    struct Object *grown;

    if (count != 0 && bytes > UINT64_MAX / count)
        return NULL;
    nbytes = bytes * count;
    if (nbytes == 0)
        nbytes = 1;
    ptr = malloc((size_t)nbytes);
    if (ptr == NULL)
        return NULL;
    if (object_count == object_cap) {
        int cap = object_cap == 0 ? 8 : object_cap * 2;
        grown = realloc(objects, (size_t)cap * sizeof *grown);
        if (grown == NULL) {
            free(ptr);
            return NULL;
        }
        objects = grown;
        object_cap = cap;
    }
    objects[object_count].base = (uintptr_t)ptr;
    objects[object_count].size = nbytes;
    objects[object_count].dtor = NULL;
    objects[object_count].marked = 0;
    object_count++;
    sorted_valid = 0;
    note_span((uintptr_t)ptr, nbytes);
    live_bytes += nbytes;
    fresh_bytes += nbytes;
    return ptr;
}

void smallcpp_set_dtor(void *ptr, void (*dtor)(void *))
{
    int index;

    if (ptr == NULL)
        return;
    index = lookup_base((uintptr_t)ptr, 1);
    if (index >= 0)
        objects[index].dtor = dtor;
}

void smallcpp_dtor_done(void *ptr)
{
    int index;

    if (ptr == NULL)
        return;
    index = lookup_base((uintptr_t)ptr, 0);
    if (index >= 0)
        objects[index].dtor = NULL;
}

static void release_point(int index, int keep_roots)
{
    if (keep_roots) {
        kept_roots = points[index].traced;
        kept_root_n = points[index].traced_count;
        points[index].traced = NULL;
    }
    free(points[index].traced);
    points[index].traced = NULL;
    points[index].traced_count = 0;
    points[index].traced_cap = 0;
    if (index == point_count - 1)
        point_count--;
}

void *smallcpp_alloc_at(uint64_t bytes, uint64_t count, uint64_t caller_rbp, uint64_t retaddr,
                        uint64_t caller_rsp)
{
    struct Safepoint *grown;
    int index = -1;
    int nested;
    int do_collect;
    void *result;

    nested = collecting;
    do_collect = !nested && should_collect();
    if (do_collect) {
        if (point_count == point_cap) {
            int cap = point_cap == 0 ? 8 : point_cap * 2;
            grown = realloc(points, (size_t)cap * sizeof *grown);
            if (grown == NULL)
                return NULL;
            points = grown;
            point_cap = cap;
        }
        index = point_count++;
        memset(&points[index], 0, sizeof points[index]);
        smallcpp_trace(index, caller_rbp, retaddr, caller_rsp);
        collecting = 1;
        reclaim(index);
        collecting = 0;
    }
    result = heap_alloc(bytes, count);
    if (!nested && result != NULL) {
        if (allocs < 2)
            kept_result[allocs] = result;
        if (do_collect)
            release_point(index, allocs == 1);
        allocs++;
    } else if (do_collect) {
        release_point(index, 0);
    }
    return result;
}

/*
 * The caller's frame pointer is still in %rbp on entry. Capture it before this
 * function builds a frame of its own.
 */
__attribute__((naked)) void *smallcpp_alloc(uint64_t bytes, uint64_t count)
{
    __asm__ volatile(
        "pushq %rbp\n\t"
        "movq %rsp, %rbp\n\t"
        "movq (%rbp), %rdx\n\t"
        "movq 8(%rbp), %rcx\n\t"
        "leaq 16(%rbp), %r8\n\t"
        "call smallcpp_alloc_at\n\t"
        "popq %rbp\n\t"
        "ret\n\t");
}

int smallcpp_safepoint_count(void)
{
    return allocs;
}

void *smallcpp_safepoint_result(int index)
{
    if (index < 0 || index >= allocs || index >= 2)
        return NULL;
    return kept_result[index];
}

int smallcpp_safepoint_traced(int index, void *ptr)
{
    int i;

    if (index != 1 || ptr == NULL)
        return 0;
    for (i = 0; i < kept_root_n; i++) {
        if (kept_roots[i] == (uintptr_t)ptr)
            return 1;
    }
    return 0;
}
