/*
 * Trace heap pointers recorded in the stack map at this safepoint.
 * A direct location in the caller's frame is the address of a pointer local.
 */
#include "internal.h"

#include <stdint.h>
#include <string.h>

enum { LocDirect = 2, LocIndirect = 3 };

static uint64_t read_u16(const uint8_t *p)
{
    uint16_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

static uint64_t read_u32(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

static uint64_t read_u64(const uint8_t *p)
{
    uint64_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

static int32_t read_i32(const uint8_t *p)
{
    int32_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

static uint64_t align8(uint64_t value)
{
    return (value + 7u) & ~7u;
}

static int in_frame(uint64_t addr, uint64_t caller_rbp, uint64_t caller_rsp)
{
    return addr >= caller_rsp && addr < caller_rbp && (addr % 8u) == 0;
}

static void consider(int index, uint64_t value)
{
    if (smallcpp_heap_contains(value))
        smallcpp_trace_pointer(index, value);
}

static void trace_location(int index, uint8_t kind, uint16_t reg, int32_t offset, uint64_t caller_rbp,
                           uint64_t caller_rsp)
{
    uint64_t base;
    uint64_t addr;
    uint64_t loaded;

    if (kind != LocDirect && kind != LocIndirect)
        return;
    if (reg == 6)
        base = caller_rbp;
    else if (reg == 7)
        base = caller_rsp;
    else
        return;
    addr = base + (int64_t)offset;
    if (kind == LocIndirect || in_frame(addr, caller_rbp, caller_rsp)) {
        if (!in_frame(addr, caller_rbp, caller_rsp))
            return;
        memcpy(&loaded, (void *)addr, sizeof loaded);
        consider(index, loaded);
        return;
    }
    consider(index, addr);
}

static void trace_record(int index, const uint8_t *map, size_t size, uint64_t cursor, uint64_t fn,
                         uint64_t retaddr, uint64_t caller_rbp, uint64_t caller_rsp)
{
    uint32_t inst;
    uint16_t locations;
    uint32_t loc;

    if (cursor + 16 > size)
        return;
    inst = (uint32_t)read_u32(map + cursor + 8);
    if (retaddr != fn + inst)
        return;
    locations = (uint16_t)read_u16(map + cursor + 14);
    cursor += 16;
    for (loc = 0; loc < locations; loc++) {
        uint8_t kind;
        uint16_t reg;
        int32_t offset;

        if (cursor + 12 > size)
            return;
        kind = map[cursor];
        reg = (uint16_t)read_u16(map + cursor + 4);
        offset = read_i32(map + cursor + 8);
        trace_location(index, kind, reg, offset, caller_rbp, caller_rsp);
        cursor += 12;
    }
}

void smallcpp_trace(int index, uint64_t caller_rbp, uint64_t retaddr, uint64_t caller_rsp)
{
    const uint8_t *map;
    size_t size;
    uint32_t functions;
    uint32_t constants;
    uint32_t records;
    uint64_t fn_at;
    uint64_t cursor;
    uint32_t seen;
    uint32_t i;

    if (!smallcpp_stackmap(&map, &size) || size < 16 || map[0] != 3)
        return;
    functions = (uint32_t)read_u32(map + 4);
    constants = (uint32_t)read_u32(map + 8);
    records = (uint32_t)read_u32(map + 12);
    fn_at = 16;
    cursor = 16 + (uint64_t)functions * 24 + (uint64_t)constants * 8;
    seen = 0;
    for (i = 0; i < functions; i++) {
        uint64_t address;
        uint64_t count;
        uint64_t rec;

        if (fn_at + 24 > size)
            return;
        address = read_u64(map + fn_at);
        count = read_u64(map + fn_at + 16);
        fn_at += 24;
        for (rec = 0; rec < count && seen < records; rec++, seen++) {
            uint16_t locations;
            uint16_t liveouts;
            uint64_t live_at;

            trace_record(index, map, size, cursor, address, retaddr, caller_rbp, caller_rsp);
            if (cursor + 16 > size)
                return;
            locations = (uint16_t)read_u16(map + cursor + 14);
            cursor += 16 + (uint64_t)locations * 12;
            live_at = align8(cursor);
            if (live_at + 4 > size)
                return;
            liveouts = (uint16_t)read_u16(map + live_at + 2);
            cursor = align8(live_at + 4 + (uint64_t)liveouts * 4);
        }
    }
}
