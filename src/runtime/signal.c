/*
 * Fatal signal stack dump.
 *
 * SIGSEGV, SIGBUS, SIGFPE, SIGILL, and SIGABRT are caught with
 * sigaction, SA_SIGINFO, and SA_ONSTACK. The alternate stack is
 * allocated once at startup, so a stack overflow can still be
 * reported. The handler is async-signal-safe: it uses write and
 * tables built before the crash. It walks saved frame pointers.
 *
 * A frame whose function is in the precise GC stack map is named
 * from the symbol table loaded at startup. The source line is
 * printed when the line table loaded at startup has one. Any other
 * frame is an address. The handler then restores the default
 * disposition and raises the same signal again.
 */
#define _GNU_SOURCE

#include "internal.h"

#include <elf.h>
#include <link.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

enum { NameCap = 256, MaxFrames = 64, AltBytes = 32 * 1024, MaxSpans = 256 };

struct Func {
    uint64_t start;
    uint64_t end;
    const char *name;
    int global;
};

struct Span {
    uint64_t lo;
    uint64_t hi;
};

static struct Func *funcs;
static int func_count;
static struct SmallcppLine *lines;
static int line_count;
static struct Span spans[MaxSpans];
static int span_count;
static void *alt_block;
static unsigned char alt_fallback[AltBytes];
static int alt_ready;
static volatile sig_atomic_t dumping;

static uint32_t read_u32(const uint8_t *p)
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

static int cmp_func(const void *left, const void *right)
{
    const struct Func *a = left;
    const struct Func *b = right;

    if (a->start < b->start)
        return -1;
    if (a->start > b->start)
        return 1;
    return 0;
}

static int map_functions(uint64_t **out, int *count)
{
    const uint8_t *bytes;
    size_t size;
    uint32_t functions;
    uint32_t i;
    uint64_t at;
    uint64_t *addrs;

    *out = NULL;
    *count = 0;
    if (!smallcpp_stackmap(&bytes, &size))
        return 0;
    if (bytes == NULL || size < 16 || bytes[0] != 3)
        return 0;
    functions = read_u32(bytes + 4);
    if (functions == 0 || (uint64_t)functions > (size - 16) / 24)
        return 0;
    addrs = malloc((size_t)functions * sizeof *addrs);
    if (addrs == NULL)
        return 0;
    at = 16;
    for (i = 0; i < functions; i++) {
        addrs[i] = read_u64(bytes + at);
        at += 24;
    }
    *out = addrs;
    *count = (int)functions;
    return 1;
}

static int contains_addr(const uint64_t *addrs, int count, uint64_t addr)
{
    int lo = 0;
    int hi = count;

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;

        if (addrs[mid] < addr)
            lo = mid + 1;
        else if (addrs[mid] > addr)
            hi = mid;
        else
            return 1;
    }
    return 0;
}

static char *copy_name(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL)
        return NULL;
    length = strlen(text);
    if (length == 0 || length >= NameCap)
        return NULL;
    copy = malloc(length + 1);
    if (copy == NULL)
        return NULL;
    memcpy(copy, text, length + 1);
    return copy;
}

static int find_start(uint64_t start)
{
    int i;

    for (i = 0; i < func_count; i++) {
        if (funcs[i].start == start)
            return i;
    }
    return -1;
}

static void add_func(uint64_t start, uint64_t end, const char *text,
                      int global)
{
    char *name;
    struct Func *grown;
    int at;

    if (end <= start)
        return;
    name = copy_name(text);
    if (name == NULL)
        return;
    at = find_start(start);
    if (at >= 0) {
        if (funcs[at].global && !global) {
            free(name);
            return;
        }
        free((void *)funcs[at].name);
        funcs[at].end = end;
        funcs[at].name = name;
        funcs[at].global = global;
        return;
    }
    grown = realloc(funcs, (size_t)(func_count + 1) * sizeof *grown);
    if (grown == NULL) {
        free(name);
        return;
    }
    funcs = grown;
    funcs[func_count].start = start;
    funcs[func_count].end = end;
    funcs[func_count].name = name;
    funcs[func_count].global = global;
    func_count++;
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

static int slurp_exe(uint8_t **bytes, size_t *size)
{
    FILE *in;
    long end;

    *bytes = NULL;
    *size = 0;
    in = fopen("/proc/self/exe", "rb");
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
    *bytes = malloc((size_t)end);
    if (*bytes == NULL) {
        fclose(in);
        return 0;
    }
    if (fread(*bytes, 1, (size_t)end, in) != (size_t)end) {
        free(*bytes);
        *bytes = NULL;
        fclose(in);
        return 0;
    }
    fclose(in);
    *size = (size_t)end;
    return 1;
}

static const Elf64_Shdr *elf_sections(const uint8_t *bytes, size_t size,
                                      uint16_t *count)
{
    const Elf64_Ehdr *eh;

    *count = 0;
    if (size < sizeof(Elf64_Ehdr))
        return NULL;
    eh = (const Elf64_Ehdr *)bytes;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0
        || eh->e_ident[EI_CLASS] != ELFCLASS64)
        return NULL;
    if (eh->e_shentsize != sizeof(Elf64_Shdr) || eh->e_shnum == 0)
        return NULL;
    if (eh->e_shoff > size
        || (uint64_t)eh->e_shnum * sizeof(Elf64_Shdr) > size - eh->e_shoff)
        return NULL;
    *count = eh->e_shnum;
    return (const Elf64_Shdr *)(bytes + eh->e_shoff);
}

static const char *sym_text(const char *strings, size_t size, uint32_t offset)
{
    size_t i;

    if ((size_t)offset >= size)
        return NULL;
    for (i = (size_t)offset; i < size; i++) {
        if (strings[i] == '\0')
            return strings + offset;
    }
    return NULL;
}

static void load_symbols(const uint64_t *mapped, int mapped_count,
                         uintptr_t bias)
{
    uint8_t *bytes = NULL;
    size_t size = 0;
    uint16_t count = 0;
    const Elf64_Ehdr *eh;
    const Elf64_Shdr *headers;
    const Elf64_Shdr *sym;
    const Elf64_Shdr *str;
    const char *names;
    const Elf64_Sym *rows;
    size_t nrows;
    size_t i;

    if (!slurp_exe(&bytes, &size))
        return;
    headers = elf_sections(bytes, size, &count);
    if (headers == NULL) {
        free(bytes);
        return;
    }
    eh = (const Elf64_Ehdr *)bytes;
    sym = NULL;
    for (i = 0; i < count; i++) {
        const char *label;
        const Elf64_Shdr *shstr;

        if (eh->e_shstrndx >= count)
            break;
        shstr = &headers[eh->e_shstrndx];
        if (shstr->sh_offset > size
            || shstr->sh_size > size - shstr->sh_offset)
            break;
        if (headers[i].sh_name >= shstr->sh_size)
            continue;
        label = (const char *)(bytes + shstr->sh_offset + headers[i].sh_name);
        if (strcmp(label, ".symtab") == 0)
            sym = &headers[i];
    }
    if (sym == NULL || sym->sh_link >= count
        || sym->sh_entsize != sizeof(Elf64_Sym)) {
        free(bytes);
        return;
    }
    str = &headers[sym->sh_link];
    if (sym->sh_offset > size || sym->sh_size > size - sym->sh_offset
        || str->sh_offset > size || str->sh_size > size - str->sh_offset) {
        free(bytes);
        return;
    }
    names = (const char *)(bytes + str->sh_offset);
    rows = (const Elf64_Sym *)(bytes + sym->sh_offset);
    nrows = (size_t)(sym->sh_size / sizeof(Elf64_Sym));
    for (i = 0; i < nrows; i++) {
        const char *text;
        uint64_t start;
        int global;

        if (ELF64_ST_TYPE(rows[i].st_info) != STT_FUNC)
            continue;
        if (rows[i].st_size == 0 || rows[i].st_shndx == SHN_UNDEF)
            continue;
        start = (uint64_t)rows[i].st_value + (uint64_t)bias;
        if (!contains_addr(mapped, mapped_count, start))
            continue;
        text = sym_text(names, (size_t)str->sh_size, rows[i].st_name);
        global = ELF64_ST_BIND(rows[i].st_info) == STB_GLOBAL;
        add_func(start, start + (uint64_t)rows[i].st_size, text, global);
    }
    free(bytes);
}

static void load_names(void)
{
    uint64_t *mapped = NULL;
    int mapped_count = 0;
    uintptr_t bias;
    int i;

    if (!map_functions(&mapped, &mapped_count))
        return;
    if (mapped_count > 1)
        qsort(mapped, (size_t)mapped_count, sizeof *mapped, cmp_u64);
    bias = load_bias();
    load_symbols(mapped, mapped_count, bias);
    free(mapped);
    if (func_count > 1)
        qsort(funcs, (size_t)func_count, sizeof *funcs, cmp_func);
    if (func_count == 0)
        return;
    if (!smallcpp_collect_lines(&lines, &line_count))
        return;
    for (i = 0; i < line_count; i++)
        lines[i].addr += (uint64_t)bias;
}

static void load_spans(void)
{
    FILE *in;
    char line[512];

    in = fopen("/proc/self/maps", "r");
    if (in == NULL)
        return;
    while (span_count < MaxSpans && fgets(line, sizeof line, in) != NULL) {
        unsigned long lo;
        unsigned long hi;
        char perms[8];

        if (sscanf(line, "%lx-%lx %7s", &lo, &hi, perms) != 3)
            continue;
        if (perms[0] != 'r' || hi <= lo)
            continue;
        spans[span_count].lo = (uint64_t)lo;
        spans[span_count].hi = (uint64_t)hi;
        span_count++;
    }
    fclose(in);
}

static void install_alt(void)
{
    stack_t stack;
    size_t bytes = (size_t)SIGSTKSZ;

    if (bytes < (size_t)AltBytes)
        bytes = (size_t)AltBytes;
    alt_block = malloc(bytes);
    if (alt_block != NULL) {
        stack.ss_sp = alt_block;
        stack.ss_size = bytes;
    } else {
        stack.ss_sp = alt_fallback;
        stack.ss_size = sizeof alt_fallback;
    }
    stack.ss_flags = 0;
    if (sigaltstack(&stack, NULL) == 0)
        alt_ready = 1;
}

static const struct Func *func_at(uint64_t pc)
{
    int lo = 0;
    int hi = func_count;
    int best = -1;

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;

        if (funcs[mid].start <= pc) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (best < 0 || pc >= funcs[best].end)
        return NULL;
    return &funcs[best];
}

static const struct SmallcppLine *line_at(const struct Func *fn, uint64_t pc)
{
    int lo = 0;
    int hi = line_count;
    int best = -1;

    if (fn == NULL || lines == NULL)
        return NULL;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;

        if (lines[mid].addr <= pc) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (best < 0)
        return NULL;
    if (lines[best].addr < fn->start || lines[best].addr >= fn->end)
        return NULL;
    if (lines[best].line == 0 || lines[best].file == NULL)
        return NULL;
    return &lines[best];
}

static void write_all(const char *text, size_t length)
{
    size_t off = 0;

    while (off < length) {
        ssize_t wrote = write(STDERR_FILENO, text + off, length - off);

        if (wrote <= 0)
            return;
        off += (size_t)wrote;
    }
}

static void write_str(const char *text)
{
    size_t length = 0;

    if (text == NULL)
        return;
    while (text[length] != '\0')
        length++;
    write_all(text, length);
}

static void write_addr(uint64_t value)
{
    char buf[18];
    int i;

    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 16; i++) {
        int shift = (15 - i) * 4;
        int digit = (int)((value >> shift) & 0xfu);

        buf[2 + i] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
    }
    write_all(buf, 18);
}

static void write_dec(unsigned value)
{
    char buf[16];
    int n = 0;

    if (value == 0) {
        write_str("0");
        return;
    }
    while (value > 0 && n < (int)sizeof buf) {
        buf[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (n > 0)
        write_all(&buf[--n], 1);
}

static const char *signal_name(int sig)
{
    if (sig == SIGSEGV)
        return "SIGSEGV";
    if (sig == SIGBUS)
        return "SIGBUS";
    if (sig == SIGFPE)
        return "SIGFPE";
    if (sig == SIGILL)
        return "SIGILL";
    if (sig == SIGABRT)
        return "SIGABRT";
    return "signal";
}

static int readable(uint64_t addr, size_t bytes)
{
    uint64_t end;
    int i;

    if (bytes == 0 || addr < 4096)
        return 0;
    end = addr + (uint64_t)bytes;
    if (end < addr)
        return 0;
    for (i = 0; i < span_count; i++) {
        if (addr >= spans[i].lo && end <= spans[i].hi)
            return 1;
    }
    return 0;
}

static int load_u64(uint64_t addr, uint64_t *out)
{
    const volatile uint64_t *slot;

    if (!readable(addr, sizeof(uint64_t)))
        return 0;
    slot = (const volatile uint64_t *)(uintptr_t)addr;
    *out = *slot;
    return 1;
}

static void write_frame(uint64_t pc)
{
    const struct Func *fn = func_at(pc);
    const struct SmallcppLine *line;

    if (fn == NULL) {
        write_addr(pc);
        write_str("\n");
        return;
    }
    write_str(fn->name);
    line = line_at(fn, pc);
    if (line != NULL) {
        write_str(" at ");
        write_str(line->file);
        write_str(":");
        write_dec(line->line);
    }
    write_str("\n");
}

static void dump_stack(int sig, void *context)
{
    write_str("fatal signal ");
    write_str(signal_name(sig));
    write_str("\n");
#if defined(__x86_64__)
    {
        ucontext_t *uc = context;
        uint64_t pc;
        uint64_t frame;
        int i;

        if (uc == NULL)
            return;
        pc = (uint64_t)uc->uc_mcontext.gregs[REG_RIP];
        frame = (uint64_t)uc->uc_mcontext.gregs[REG_RBP];
        for (i = 0; i < MaxFrames && pc != 0; i++) {
            uint64_t parent;
            uint64_t ret;

            write_frame(pc);
            if ((frame % 8u) != 0 || !readable(frame, 16))
                return;
            if (!load_u64(frame, &parent) || !load_u64(frame + 8, &ret))
                return;
            if (parent <= frame || parent - frame > 1024u * 1024u)
                return;
            if ((parent % 8u) != 0)
                return;
            frame = parent;
            pc = ret;
        }
    }
#else
    (void)context;
#endif
}

static void reraise(int sig)
{
    struct sigaction sa = {0};
    sigset_t set;

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
    raise(sig);
}

static void on_signal(int sig, siginfo_t *info, void *context)
{
    (void)info;
    if (dumping) {
        reraise(sig);
        return;
    }
    dumping = 1;
    dump_stack(sig, context);
    reraise(sig);
}

static const int fatal_signals[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};

static void install_handlers(void)
{
    struct sigaction sa = {0};
    int i;

    sa.sa_sigaction = on_signal;
    sigemptyset(&sa.sa_mask);
    for (i = 0; i < 5; i++)
        sigaddset(&sa.sa_mask, fatal_signals[i]);
    sa.sa_flags = SA_SIGINFO;
    if (alt_ready)
        sa.sa_flags |= SA_ONSTACK;
    for (i = 0; i < 5; i++)
        sigaction(fatal_signals[i], &sa, NULL);
}

void smallcpp_fatal_init(void)
{
    static int ready;

    if (ready)
        return;
    ready = 1;
    load_spans();
    load_names();
    install_alt();
    install_handlers();
}
