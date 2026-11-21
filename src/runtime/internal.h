#pragma once

#include <stddef.h>
#include <stdint.h>

void *smallcpp_alloc_at(uint64_t bytes, uint64_t count, uint64_t caller_rbp, uint64_t retaddr,
                        uint64_t caller_rsp);
int smallcpp_heap_contains(uintptr_t addr);
void smallcpp_trace_pointer(int index, uintptr_t addr);
int smallcpp_stackmap(const uint8_t **bytes, size_t *size);
void smallcpp_trace(int index, uint64_t caller_rbp, uint64_t retaddr, uint64_t caller_rsp);

/* One .scpp source line. addr is the link address. file lives until exit. */
struct SmallcppLine {
    uint64_t addr;
    unsigned line;
    const char *file;
};

/* Every .scpp line row in this executable. 0 when the table is empty. */
int smallcpp_collect_lines(struct SmallcppLine **rows, int *count);

/* Catch fatal signals once. Safe to call again. */
void smallcpp_fatal_init(void);
