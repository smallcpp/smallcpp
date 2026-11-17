/*
 * Find the loaded .llvm_stackmaps section of this executable.
 */
#define _GNU_SOURCE

#include "internal.h"

#include <elf.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Image {
    uintptr_t bias;
    const Elf64_Phdr *phdr;
    int phnum;
    int found;
};

static int image_phdr(struct dl_phdr_info *info, size_t size, void *data)
{
    struct Image *image = data;

    (void)size;
    image->bias = (uintptr_t)info->dlpi_addr;
    image->phdr = info->dlpi_phdr;
    image->phnum = info->dlpi_phnum;
    image->found = 1;
    return 1;
}

int smallcpp_stackmap(const uint8_t **bytes, size_t *size)
{
    static const uint8_t *cached;
    static size_t cached_size;
    static int ready;
    struct Image image;
    FILE *file;
    Elf64_Ehdr eh;
    Elf64_Shdr shstr;
    char *names;
    int i;

    if (ready) {
        *bytes = cached;
        *size = cached_size;
        return cached != NULL;
    }
    ready = 1;
    *bytes = NULL;
    *size = 0;
    memset(&image, 0, sizeof image);
    dl_iterate_phdr(image_phdr, &image);
    if (!image.found)
        return 0;
    file = fopen("/proc/self/exe", "rb");
    if (file == NULL)
        return 0;
    if (fread(&eh, sizeof eh, 1, file) != 1 || memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0
        || eh.e_ident[EI_CLASS] != ELFCLASS64 || eh.e_shoff == 0 || eh.e_shentsize != sizeof(Elf64_Shdr)) {
        fclose(file);
        return 0;
    }
    if (fseek(file, (long)(eh.e_shoff + (Elf64_Off)eh.e_shstrndx * eh.e_shentsize), SEEK_SET) != 0
        || fread(&shstr, sizeof shstr, 1, file) != 1) {
        fclose(file);
        return 0;
    }
    names = malloc(shstr.sh_size);
    if (names == NULL) {
        fclose(file);
        return 0;
    }
    if (fseek(file, (long)shstr.sh_offset, SEEK_SET) != 0 || fread(names, shstr.sh_size, 1, file) != 1) {
        free(names);
        fclose(file);
        return 0;
    }
    for (i = 0; i < eh.e_shnum; i++) {
        Elf64_Shdr sh;
        const char *name;
        int ph;

        if (fseek(file, (long)(eh.e_shoff + (Elf64_Off)i * eh.e_shentsize), SEEK_SET) != 0
            || fread(&sh, sizeof sh, 1, file) != 1)
            break;
        if (sh.sh_name >= shstr.sh_size)
            continue;
        name = names + sh.sh_name;
        if (strcmp(name, ".llvm_stackmaps") != 0)
            continue;
        for (ph = 0; ph < image.phnum; ph++) {
            const Elf64_Phdr *hdr = &image.phdr[ph];
            if (hdr->p_type != PT_LOAD)
                continue;
            if (sh.sh_offset < hdr->p_offset || sh.sh_offset >= hdr->p_offset + hdr->p_filesz)
                continue;
            cached = (const uint8_t *)(image.bias + hdr->p_vaddr + (sh.sh_offset - hdr->p_offset));
            cached_size = sh.sh_size;
            *bytes = cached;
            *size = cached_size;
            free(names);
            fclose(file);
            return 1;
        }
    }
    free(names);
    fclose(file);
    return 0;
}
