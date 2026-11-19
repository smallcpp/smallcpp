/*
 * Trace heap pointers recorded in the stack map at this safepoint.
 * A direct location in the caller's frame is the address of a pointer local.
 * Return addresses are indexed once. A caller frame with a map is traced
 * from that map. A frame with no map is still scanned.
 */
#include "internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { LocDirect = 2, LocIndirect = 3, MapWindow = 64 };

struct Site {
    uint64_t addr;
    uint64_t fn;
    uint32_t cursor;
};

static const uint8_t *map_bytes;
static size_t map_size;
static struct Site *sites;
static int site_count;
static uint64_t *fn_addrs;
static int fn_count;
static int index_ready;

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

static void scan_words(int index, uint64_t low, uint64_t high)
{
    uint64_t addr;

    if (high < low || high - low > 1024u * 1024u)
        return;
    addr = (low + 7u) & ~((uint64_t)7u);
    while (addr + sizeof(uint64_t) <= high) {
        uint64_t word;

        memcpy(&word, (void *)addr, sizeof word);
        consider(index, word);
        addr += sizeof(uint64_t);
    }
}

static void trace_record(int index, uint64_t cursor, uint64_t fn, uint64_t retaddr, uint64_t caller_rbp,
                         uint64_t caller_rsp)
{
    uint32_t inst;
    uint16_t locations;
    uint32_t loc;

    if (cursor + 16 > map_size)
        return;
    inst = (uint32_t)read_u32(map_bytes + cursor + 8);
    if (retaddr != fn + inst)
        return;
    locations = (uint16_t)read_u16(map_bytes + cursor + 14);
    cursor += 16;
    for (loc = 0; loc < locations; loc++) {
        uint8_t kind;
        uint16_t reg;
        int32_t offset;

        if (cursor + 12 > map_size)
            return;
        kind = map_bytes[cursor];
        reg = (uint16_t)read_u16(map_bytes + cursor + 4);
        offset = read_i32(map_bytes + cursor + 8);
        trace_location(index, kind, reg, offset, caller_rbp, caller_rsp);
        cursor += 12;
    }
}

static int cmp_site(const void *left, const void *right)
{
    const struct Site *a = left;
    const struct Site *b = right;

    if (a->addr < b->addr)
        return -1;
    if (a->addr > b->addr)
        return 1;
    if (a->cursor < b->cursor)
        return -1;
    if (a->cursor > b->cursor)
        return 1;
    return 0;
}

static int cmp_u64(const void *left, const void *right)
{
    uint64_t a = *(const uint64_t *)left;
    uint64_t b = *(const uint64_t *)right;

    if (a < b)
        return -1;
    if (a > b)
        return 1;
    return 0;
}

static void build_index(void)
{
    uint32_t functions;
    uint32_t constants;
    uint32_t records;
    uint64_t fn_at;
    uint64_t cursor;
    uint32_t seen;
    uint32_t i;
    int filled = 0;

    if (map_size < 16 || map_bytes[0] != 3)
        return;
    functions = (uint32_t)read_u32(map_bytes + 4);
    constants = (uint32_t)read_u32(map_bytes + 8);
    records = (uint32_t)read_u32(map_bytes + 12);
    if (records == 0 || functions == 0)
        return;
    sites = malloc((size_t)records * sizeof *sites);
    fn_addrs = malloc((size_t)functions * sizeof *fn_addrs);
    if (sites == NULL || fn_addrs == NULL) {
        free(sites);
        free(fn_addrs);
        sites = NULL;
        fn_addrs = NULL;
        return;
    }
    fn_at = 16;
    cursor = 16 + (uint64_t)functions * 24 + (uint64_t)constants * 8;
    seen = 0;
    for (i = 0; i < functions; i++) {
        uint64_t address;
        uint64_t count;
        uint64_t rec;

        if (fn_at + 24 > map_size)
            break;
        address = read_u64(map_bytes + fn_at);
        count = read_u64(map_bytes + fn_at + 16);
        fn_addrs[fn_count++] = address;
        fn_at += 24;
        for (rec = 0; rec < count && seen < records; rec++, seen++) {
            uint16_t locations;
            uint16_t liveouts;
            uint64_t live_at;
            uint32_t inst;

            if (cursor + 16 > map_size)
                goto done;
            inst = (uint32_t)read_u32(map_bytes + cursor + 8);
            sites[filled].addr = address + inst;
            sites[filled].fn = address;
            sites[filled].cursor = (uint32_t)cursor;
            filled++;
            locations = (uint16_t)read_u16(map_bytes + cursor + 14);
            cursor += 16 + (uint64_t)locations * 12;
            live_at = align8(cursor);
            if (live_at + 4 > map_size)
                goto done;
            liveouts = (uint16_t)read_u16(map_bytes + live_at + 2);
            cursor = align8(live_at + 4 + (uint64_t)liveouts * 4);
        }
    }
done:
    site_count = filled;
    if (site_count > 1)
        qsort(sites, (size_t)site_count, sizeof *sites, cmp_site);
    if (fn_count > 1)
        qsort(fn_addrs, (size_t)fn_count, sizeof *fn_addrs, cmp_u64);
}

static void ensure_index(void)
{
    if (index_ready)
        return;
    index_ready = 1;
    if (!smallcpp_stackmap(&map_bytes, &map_size))
        return;
    build_index();
}

static uint64_t function_of(uint64_t retaddr)
{
    int lo;
    int hi;
    int best;

    lo = 0;
    hi = fn_count;
    best = -1;
    while (lo < hi) {
        int mid;

        mid = lo + (hi - lo) / 2;
        if (fn_addrs[mid] <= retaddr) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (best < 0)
        return 0;
    return fn_addrs[best];
}

static int lower_bound(uint64_t addr)
{
    int lo;
    int hi;

    lo = 0;
    hi = site_count;
    while (lo < hi) {
        int mid;

        mid = lo + (hi - lo) / 2;
        if (sites[mid].addr < addr)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

static void trace_sites(int index, int at, uint64_t rbp, uint64_t rsp)
{
    uint64_t addr;

    if (at < 0 || at >= site_count)
        return;
    addr = sites[at].addr;
    while (at < site_count && sites[at].addr == addr) {
        trace_record(index, sites[at].cursor, sites[at].fn, addr, rbp, rsp);
        at++;
    }
}

/* 1 when a stack map covers this return address. */
static int trace_return(int index, uint64_t retaddr, uint64_t rbp, uint64_t rsp)
{
    int at;
    uint64_t fn;

    if (sites == NULL || site_count == 0 || retaddr == 0)
        return 0;
    at = lower_bound(retaddr);
    if (at < site_count && sites[at].addr == retaddr) {
        trace_sites(index, at, rbp, rsp);
        return 1;
    }
    fn = function_of(retaddr);
    if (fn == 0 || at >= site_count || sites[at].fn != fn)
        return 0;
    if (sites[at].addr - retaddr > MapWindow)
        return 0;
    trace_sites(index, at, rbp, rsp);
    return 1;
}

static void trace_linear(int index, uint64_t retaddr, uint64_t caller_rbp, uint64_t caller_rsp)
{
    uint32_t functions;
    uint32_t constants;
    uint32_t records;
    uint64_t fn_at;
    uint64_t cursor;
    uint32_t seen;
    uint32_t i;

    if (map_bytes == NULL || map_size < 16 || map_bytes[0] != 3)
        return;
    functions = (uint32_t)read_u32(map_bytes + 4);
    constants = (uint32_t)read_u32(map_bytes + 8);
    records = (uint32_t)read_u32(map_bytes + 12);
    fn_at = 16;
    cursor = 16 + (uint64_t)functions * 24 + (uint64_t)constants * 8;
    seen = 0;
    for (i = 0; i < functions; i++) {
        uint64_t address;
        uint64_t count;
        uint64_t rec;

        if (fn_at + 24 > map_size)
            return;
        address = read_u64(map_bytes + fn_at);
        count = read_u64(map_bytes + fn_at + 16);
        fn_at += 24;
        for (rec = 0; rec < count && seen < records; rec++, seen++) {
            uint16_t locations;
            uint16_t liveouts;
            uint64_t live_at;

            trace_record(index, cursor, address, retaddr, caller_rbp, caller_rsp);
            if (cursor + 16 > map_size)
                return;
            locations = (uint16_t)read_u16(map_bytes + cursor + 14);
            cursor += 16 + (uint64_t)locations * 12;
            live_at = align8(cursor);
            if (live_at + 4 > map_size)
                return;
            liveouts = (uint16_t)read_u16(map_bytes + live_at + 2);
            cursor = align8(live_at + 4 + (uint64_t)liveouts * 4);
        }
    }
}

static void scan_parents(int index, uint64_t frame)
{
    int guard;

    for (guard = 0; guard < 32; guard++) {
        uint64_t parent;
        uint64_t ret;

        if (frame < 4096 || (frame % 8u) != 0)
            return;
        memcpy(&parent, (void *)frame, sizeof parent);
        if (parent <= frame || parent - frame > 1024u * 1024u || (parent % 8u) != 0)
            return;
        memcpy(&ret, (void *)(frame + 8), sizeof ret);
        if (!trace_return(index, ret, parent, frame + 16))
            scan_words(index, frame + 16, parent);
        frame = parent;
    }
}

void smallcpp_trace(int index, uint64_t caller_rbp, uint64_t retaddr, uint64_t caller_rsp)
{
    ensure_index();
    if (sites != NULL)
        trace_return(index, retaddr, caller_rbp, caller_rsp);
    else
        trace_linear(index, retaddr, caller_rbp, caller_rsp);
    scan_parents(index, caller_rbp);
}
