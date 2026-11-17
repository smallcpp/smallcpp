/*
 * Call stack for an uncaught exception.
 *
 * Frames come from backtrace. Line numbers come from the DWARF line
 * table the compiler writes with -g. Only frames in a .scpp file are
 * printed; runtime and C library frames have no such line.
 */
#define _GNU_SOURCE

#include <elf.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

enum { MaxFiles = 64, MaxFrames = 64, NameCap = 256 };

struct Image {
    uint8_t *bytes;
    size_t size;
};

struct Cur {
    const uint8_t *p;
    size_t n;
    size_t i;
    int bad;
};

struct Row {
    uint64_t address;
    const char *file;
    unsigned line;
};

static int slurp(struct Image *image)
{
    FILE *in = fopen("/proc/self/exe", "rb");
    long end;

    image->bytes = NULL;
    image->size = 0;
    if (in == NULL)
        return 0;
    if (fseek(in, 0, SEEK_END) != 0) {
        fclose(in);
        return 0;
    }
    end = ftell(in);
    if (end <= 0 || end > 64 * 1024 * 1024) {
        fclose(in);
        return 0;
    }
    if (fseek(in, 0, SEEK_SET) != 0) {
        fclose(in);
        return 0;
    }
    image->bytes = malloc((size_t)end);
    if (image->bytes == NULL) {
        fclose(in);
        return 0;
    }
    if (fread(image->bytes, 1, (size_t)end, in) != (size_t)end) {
        free(image->bytes);
        image->bytes = NULL;
        fclose(in);
        return 0;
    }
    fclose(in);
    image->size = (size_t)end;
    return 1;
}

static int range(const struct Image *image, uint64_t off, uint64_t len)
{
    return len <= image->size && off <= image->size - len;
}

static const Elf64_Shdr *sections(const struct Image *image, uint16_t *count)
{
    const Elf64_Ehdr *eh;

    *count = 0;
    if (image->size < sizeof(Elf64_Ehdr))
        return NULL;
    eh = (const Elf64_Ehdr *)image->bytes;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64)
        return NULL;
    if (eh->e_shentsize != sizeof(Elf64_Shdr) || eh->e_shnum == 0)
        return NULL;
    if (!range(image, eh->e_shoff, (uint64_t)eh->e_shnum * sizeof(Elf64_Shdr)))
        return NULL;
    *count = eh->e_shnum;
    return (const Elf64_Shdr *)(image->bytes + eh->e_shoff);
}

static const char *shstr(const struct Image *image, const Elf64_Shdr *headers, uint16_t count)
{
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)image->bytes;
    const Elf64_Shdr *str;

    if (eh->e_shstrndx >= count)
        return NULL;
    str = &headers[eh->e_shstrndx];
    if (!range(image, str->sh_offset, str->sh_size))
        return NULL;
    return (const char *)(image->bytes + str->sh_offset);
}

static const Elf64_Shdr *section_named(const struct Image *image, const char *name)
{
    uint16_t count = 0;
    const Elf64_Shdr *headers = sections(image, &count);
    const char *names;
    uint16_t i;

    if (headers == NULL)
        return NULL;
    names = shstr(image, headers, count);
    if (names == NULL)
        return NULL;
    for (i = 0; i < count; i++) {
        if (headers[i].sh_name >= headers[((const Elf64_Ehdr *)image->bytes)->e_shstrndx].sh_size)
            continue;
        if (strcmp(names + headers[i].sh_name, name) == 0)
            return &headers[i];
    }
    return NULL;
}

static int bias_visit(struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    if (info->dlpi_name != NULL && info->dlpi_name[0] != '\0')
        return 0;
    *(uintptr_t *)data = (uintptr_t)info->dlpi_addr;
    return 1;
}

static uintptr_t load_bias(void)
{
    uintptr_t bias = 0;
    dl_iterate_phdr(bias_visit, &bias);
    return bias;
}

static uint8_t take_u8(struct Cur *c)
{
    if (c->i >= c->n) {
        c->bad = 1;
        return 0;
    }
    return c->p[c->i++];
}

static uint64_t take_uleb(struct Cur *c)
{
    uint64_t value = 0;
    int shift = 0;

    while (c->i < c->n && shift <= 56) {
        uint8_t byte = c->p[c->i++];
        value |= (uint64_t)(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0)
            return value;
        shift += 7;
    }
    c->bad = 1;
    return 0;
}

static int64_t take_sleb(struct Cur *c)
{
    uint64_t value = 0;
    int shift = 0;
    uint8_t byte = 0;

    while (c->i < c->n && shift <= 56) {
        byte = c->p[c->i++];
        value |= (uint64_t)(byte & 0x7f) << shift;
        shift += 7;
        if ((byte & 0x80) == 0)
            break;
    }
    if ((byte & 0x80) != 0 || shift == 0) {
        c->bad = 1;
        return 0;
    }
    if (shift < 64 && (byte & 0x40) != 0)
        value |= ~0ULL << shift;
    return (int64_t)value;
}

static uint64_t take_n(struct Cur *c, size_t n)
{
    uint64_t value = 0;
    size_t i;

    if (c->i + n > c->n) {
        c->bad = 1;
        return 0;
    }
    for (i = 0; i < n; i++)
        value |= (uint64_t)c->p[c->i++] << (8 * i);
    return value;
}

static const char *take_str(struct Cur *c)
{
    const char *text;
    size_t i;

    if (c->i >= c->n) {
        c->bad = 1;
        return "";
    }
    text = (const char *)(c->p + c->i);
    for (i = c->i; i < c->n; i++) {
        if (c->p[i] == '\0') {
            c->i = i + 1;
            return text;
        }
    }
    c->bad = 1;
    c->i = c->n;
    return "";
}

static int scpp_file(const char *name)
{
    size_t n;

    if (name == NULL)
        return 0;
    n = strlen(name);
    return n >= 5 && strcmp(name + n - 5, ".scpp") == 0;
}

static void consider(struct Row *best, int *found, struct Row row, uint64_t target)
{
    if (row.file == NULL || !scpp_file(row.file) || row.line == 0)
        return;
    if (row.address > target)
        return;
    if (*found && row.address < best->address)
        return;
    *best = row;
    *found = 1;
}

static void parse_unit(struct Cur *c, size_t unit_end, uint64_t target, struct Row *best, int *found)
{
    uint16_t version;
    uint32_t prologue;
    size_t program;
    uint8_t min_inst;
    uint8_t max_ops;
    int8_t line_base;
    uint8_t line_range;
    uint8_t opcode_base;
    uint8_t lengths[256];
    const char *files[MaxFiles];
    unsigned nfiles = 1;
    uint64_t address = 0;
    int line = 1;
    unsigned file = 1;
    int skip = 0;
    struct Row cover;
    int cover_ok = 0;
    unsigned i;

    memset(files, 0, sizeof files);
    memset(lengths, 0, sizeof lengths);
    if (c->i + 2 + 4 > unit_end) {
        c->bad = 1;
        return;
    }
    version = (uint16_t)take_n(c, 2);
    prologue = (uint32_t)take_n(c, 4);
    if (c->bad || version != 4) {
        c->i = unit_end;
        c->bad = 0;
        return;
    }
    program = c->i + prologue;
    if (program > unit_end) {
        c->i = unit_end;
        return;
    }
    min_inst = take_u8(c);
    max_ops = take_u8(c);
    (void)take_u8(c);
    line_base = (int8_t)take_u8(c);
    line_range = take_u8(c);
    opcode_base = take_u8(c);
    if (c->bad || min_inst == 0 || max_ops != 1 || line_range == 0 || opcode_base == 0) {
        c->i = unit_end;
        c->bad = 0;
        return;
    }
    for (i = 1; i < opcode_base && i < 256; i++)
        lengths[i] = take_u8(c);
    while (!c->bad && c->i < program) {
        const char *dir = take_str(c);
        if (c->bad || dir[0] == '\0')
            break;
    }
    while (!c->bad && c->i < program) {
        const char *name = take_str(c);
        uint64_t index;
        if (c->bad || name[0] == '\0')
            break;
        (void)take_uleb(c);
        (void)take_uleb(c);
        (void)take_uleb(c);
        index = nfiles;
        if (index < MaxFiles) {
            files[index] = name;
            nfiles++;
        }
    }
    if (c->bad) {
        c->i = unit_end;
        c->bad = 0;
        return;
    }
    c->i = program;
    while (!c->bad && c->i < unit_end) {
        uint8_t op = take_u8(c);
        if (op == 0) {
            uint64_t len = take_uleb(c);
            size_t start;
            uint8_t ext;
            if (c->bad || len == 0 || c->i + len > unit_end)
                break;
            start = c->i;
            ext = take_u8(c);
            if (ext == 1) {
                if (!skip && cover_ok && address > target)
                    consider(best, found, cover, target);
                cover_ok = 0;
                skip = 0;
                address = 0;
                line = 1;
                file = 1;
            } else if (ext == 2 && len >= 9) {
                address = take_n(c, 8);
            }
            c->i = start + len;
            continue;
        }
        if (op < opcode_base) {
            if (op == 1) {
                struct Row row;
                row.address = address;
                row.line = line > 0 ? (unsigned)line : 0;
                row.file = file < nfiles ? files[file] : NULL;
                if (!skip) {
                    if (address <= target) {
                        cover = row;
                        cover_ok = 1;
                    } else {
                        if (cover_ok)
                            consider(best, found, cover, target);
                        cover_ok = 0;
                        skip = 1;
                    }
                }
            } else if (op == 2) {
                address += take_uleb(c) * min_inst;
            } else if (op == 3) {
                line += (int)take_sleb(c);
            } else if (op == 4) {
                file = (unsigned)take_uleb(c);
            } else if (op == 5) {
                (void)take_uleb(c);
            } else if (op == 8) {
                uint8_t adjusted = (uint8_t)(255 - opcode_base);
                address += (uint64_t)(adjusted / line_range) * min_inst;
            } else if (op == 9) {
                address += take_n(c, 2);
            } else {
                uint8_t count = lengths[op];
                uint8_t n;
                for (n = 0; n < count; n++)
                    (void)take_uleb(c);
            }
            continue;
        }
        {
            uint8_t adjusted = (uint8_t)(op - opcode_base);
            struct Row row;
            const char *name;
            address += (uint64_t)(adjusted / line_range) * min_inst;
            line += line_base + (int)(adjusted % line_range);
            name = file < nfiles ? files[file] : NULL;
            row.address = address;
            row.line = line > 0 ? (unsigned)line : 0;
            row.file = name;
            if (!skip) {
                if (address <= target) {
                    cover = row;
                    cover_ok = 1;
                } else {
                    if (cover_ok)
                        consider(best, found, cover, target);
                    cover_ok = 0;
                    skip = 1;
                }
            }
        }
    }
    if (!skip && cover_ok)
        consider(best, found, cover, target);
    c->i = unit_end;
    c->bad = 0;
}

static int lookup_line(const struct Image *image, uint64_t target, char *file, unsigned *line)
{
    const Elf64_Shdr *sec = section_named(image, ".debug_line");
    struct Cur cur;
    struct Row best;
    int found = 0;

    file[0] = '\0';
    *line = 0;
    if (sec == NULL || !range(image, sec->sh_offset, sec->sh_size))
        return 0;
    cur.p = image->bytes + sec->sh_offset;
    cur.n = (size_t)sec->sh_size;
    cur.i = 0;
    cur.bad = 0;
    memset(&best, 0, sizeof best);
    while (!cur.bad && cur.i + 4 <= cur.n) {
        uint32_t length = (uint32_t)take_n(&cur, 4);
        size_t unit_end;
        if (length == 0xffffffffu) {
            uint64_t long_len = take_n(&cur, 8);
            if (cur.bad || long_len > cur.n - cur.i)
                break;
            cur.i += (size_t)long_len;
            continue;
        }
        if (length > cur.n - cur.i)
            break;
        unit_end = cur.i + length;
        parse_unit(&cur, unit_end, target, &best, &found);
        cur.i = unit_end;
        cur.bad = 0;
    }
    if (!found || best.file == NULL)
        return 0;
    snprintf(file, NameCap, "%s", best.file);
    *line = best.line;
    return file[0] != '\0' && *line != 0;
}

static int lookup_name(const struct Image *image, uint64_t target, char *name)
{
    const Elf64_Shdr *sym = section_named(image, ".symtab");
    const Elf64_Shdr *headers;
    uint16_t count = 0;
    const char *strings;
    const Elf64_Sym *rows;
    uint64_t best = ~0ULL;
    size_t i;
    size_t nrows;

    name[0] = '\0';
    if (sym == NULL || sym->sh_link == 0 || sym->sh_entsize != sizeof(Elf64_Sym))
        return 0;
    headers = sections(image, &count);
    if (headers == NULL || sym->sh_link >= count)
        return 0;
    if (!range(image, headers[sym->sh_link].sh_offset, headers[sym->sh_link].sh_size))
        return 0;
    if (!range(image, sym->sh_offset, sym->sh_size))
        return 0;
    strings = (const char *)(image->bytes + headers[sym->sh_link].sh_offset);
    rows = (const Elf64_Sym *)(image->bytes + sym->sh_offset);
    nrows = (size_t)(sym->sh_size / sizeof(Elf64_Sym));
    for (i = 0; i < nrows; i++) {
        const char *text;
        if (ELF64_ST_TYPE(rows[i].st_info) != STT_FUNC || rows[i].st_size == 0)
            continue;
        if (rows[i].st_value > target || target >= rows[i].st_value + rows[i].st_size)
            continue;
        if (rows[i].st_name >= headers[sym->sh_link].sh_size)
            continue;
        text = strings + rows[i].st_name;
        if (text[0] == '\0' || rows[i].st_size >= best)
            continue;
        best = rows[i].st_size;
        snprintf(name, NameCap, "%s", text);
    }
    return name[0] != '\0';
}

void smallcpp_print_stack(FILE *out)
{
    void *frames[MaxFrames];
    int count;
    struct Image image;
    uintptr_t bias;
    int i;

    if (out == NULL)
        return;
    count = backtrace(frames, MaxFrames);
    if (count <= 0 || !slurp(&image))
        return;
    bias = load_bias();
    for (i = 0; i < count; i++) {
        uintptr_t pc = (uintptr_t)frames[i];
        uint64_t addr;
        char file[NameCap];
        char name[NameCap];
        unsigned line = 0;

        if (pc == 0)
            continue;
        pc -= 1;
        if (pc < bias)
            continue;
        addr = (uint64_t)(pc - bias);
        if (!lookup_line(&image, addr, file, &line))
            continue;
        if (lookup_name(&image, addr, name))
            fprintf(out, "%s at %s:%u\n", name, file, line);
        else
            fprintf(out, "%s:%u\n", file, line);
    }
    free(image.bytes);
}
