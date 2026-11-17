/*
 * Allocator declared for new. delete does not free.
 */
#include "internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Object {
    uintptr_t base;
    uint64_t size;
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
static struct Safepoint *points;
static int point_count;
static int point_cap;

int smallcpp_heap_contains(uintptr_t addr)
{
    int i;

    for (i = 0; i < object_count; i++) {
        if (addr >= objects[i].base && addr < objects[i].base + objects[i].size)
            return 1;
    }
    return 0;
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
    object_count++;
    return ptr;
}

void *smallcpp_alloc_at(uint64_t bytes, uint64_t count, uint64_t caller_rbp, uint64_t retaddr,
                        uint64_t caller_rsp)
{
    struct Safepoint *grown;
    int index;

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
    points[index].result = heap_alloc(bytes, count);
    return points[index].result;
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
    return point_count;
}

void *smallcpp_safepoint_result(int index)
{
    if (index < 0 || index >= point_count)
        return NULL;
    return points[index].result;
}

int smallcpp_safepoint_traced(int index, void *ptr)
{
    int i;

    if (index < 0 || index >= point_count || ptr == NULL)
        return 0;
    for (i = 0; i < points[index].traced_count; i++) {
        if (points[index].traced[i] == (uintptr_t)ptr)
            return 1;
    }
    return 0;
}
