/*
 * The system linker concatenates .llvm_stackmaps and leaves each
 * object's header in place. The collector reads one header, so a
 * safepoint in a later object is invisible. Join those headers
 * after the link. A single map is left as the linker wrote it.
 */
#include "stackmap.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <vector>

namespace {

bool read_u16(const std::vector<uint8_t> &bytes, size_t offset, uint16_t *value)
{
    if (offset + sizeof(uint16_t) > bytes.size())
        return false;
    std::memcpy(value, bytes.data() + offset, sizeof(uint16_t));
    return true;
}

bool read_u32(const std::vector<uint8_t> &bytes, size_t offset, uint32_t *value)
{
    if (offset + sizeof(uint32_t) > bytes.size())
        return false;
    std::memcpy(value, bytes.data() + offset, sizeof(uint32_t));
    return true;
}

bool read_u64(const std::vector<uint8_t> &bytes, size_t offset, uint64_t *value)
{
    if (offset + sizeof(uint64_t) > bytes.size())
        return false;
    std::memcpy(value, bytes.data() + offset, sizeof(uint64_t));
    return true;
}

void write_u32(std::vector<uint8_t> &bytes, size_t offset, uint32_t value)
{
    std::memcpy(bytes.data() + offset, &value, sizeof value);
}

size_t align8(size_t value)
{
    return (value + 7u) & ~size_t{7};
}

struct Map {
    uint32_t functions;
    uint32_t constants;
    uint32_t records;
    size_t function_at;
    size_t constant_at;
    size_t record_at;
    size_t end;
};

bool parse_map(const std::vector<uint8_t> &bytes, size_t start, Map *map)
{
    uint32_t functions;
    uint32_t constants;
    uint32_t records;
    size_t cursor;
    size_t seen;
    size_t function;

    if (start + 16 > bytes.size() || bytes[start] != 3)
        return false;
    if (!read_u32(bytes, start + 4, &functions) || !read_u32(bytes, start + 8, &constants)
        || !read_u32(bytes, start + 12, &records))
        return false;
    if (functions > bytes.size() / 24 || constants > bytes.size() / 8)
        return false;
    cursor = start + 16 + static_cast<size_t>(functions) * 24 + static_cast<size_t>(constants) * 8;
    if (cursor > bytes.size())
        return false;
    seen = 0;
    function = start + 16;
    for (uint32_t index = 0; index < functions; ++index) {
        uint64_t count;

        if (!read_u64(bytes, function + 16, &count))
            return false;
        function += 24;
        for (uint64_t record = 0; record < count; ++record) {
            uint16_t locations;
            uint16_t liveouts;
            size_t body;
            size_t live_at;

            if (seen >= records || cursor + 16 > bytes.size())
                return false;
            if (!read_u16(bytes, cursor + 14, &locations))
                return false;
            if (locations > (bytes.size() - cursor) / 12)
                return false;
            body = cursor + 16 + static_cast<size_t>(locations) * 12;
            if (body > bytes.size())
                return false;
            live_at = align8(body);
            if (live_at + 4 > bytes.size() || !read_u16(bytes, live_at + 2, &liveouts))
                return false;
            if (liveouts > (bytes.size() - live_at) / 4)
                return false;
            cursor = align8(live_at + 4 + static_cast<size_t>(liveouts) * 4);
            if (cursor > bytes.size())
                return false;
            ++seen;
        }
    }
    if (seen != records)
        return false;
    map->functions = functions;
    map->constants = constants;
    map->records = records;
    map->function_at = start + 16;
    map->constant_at = start + 16 + static_cast<size_t>(functions) * 24;
    map->record_at = map->constant_at + static_cast<size_t>(constants) * 8;
    map->end = cursor;
    return true;
}

void append_span(std::vector<uint8_t> &out, const std::vector<uint8_t> &bytes, size_t at, size_t size)
{
    out.insert(out.end(), bytes.begin() + static_cast<std::ptrdiff_t>(at),
               bytes.begin() + static_cast<std::ptrdiff_t>(at + size));
}

bool remainder_clear(const std::vector<uint8_t> &bytes, size_t at)
{
    for (size_t index = at; index < bytes.size(); ++index) {
        if (bytes[index] != 0)
            return false;
    }
    return true;
}

} // namespace

int merge_stackmaps(const std::string &path)
{
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    Elf64_Ehdr header;
    std::vector<Elf64_Shdr> sections;
    std::vector<char> names;
    Elf64_Off section_offset = 0;
    uint64_t section_size = 0;
    bool found = false;
    std::streamoff file_size;

    if (!file)
        return 0;
    file.seekg(0, std::ios::end);
    file_size = file.tellg();
    file.seekg(0);
    if (!file || file_size < static_cast<std::streamoff>(sizeof header))
        return 0;
    file.read(reinterpret_cast<char *>(&header), sizeof header);
    if (!file || std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0 || header.e_ident[EI_CLASS] != ELFCLASS64
        || header.e_ident[EI_DATA] != ELFDATA2LSB || header.e_shoff == 0
        || header.e_shentsize != sizeof(Elf64_Shdr) || header.e_shnum == 0
        || header.e_shstrndx >= header.e_shnum)
        return 0;
    if (header.e_shoff > static_cast<Elf64_Off>(file_size)
        || static_cast<uint64_t>(header.e_shnum) * sizeof(Elf64_Shdr)
            > static_cast<uint64_t>(file_size) - header.e_shoff)
        return 0;
    sections.resize(header.e_shnum);
    file.seekg(static_cast<std::streamoff>(header.e_shoff));
    file.read(reinterpret_cast<char *>(sections.data()),
              static_cast<std::streamsize>(sections.size() * sizeof(Elf64_Shdr)));
    if (!file)
        return 0;
    {
        const Elf64_Shdr &names_header = sections[header.e_shstrndx];

        if (names_header.sh_size == 0 || names_header.sh_offset > static_cast<Elf64_Off>(file_size)
            || names_header.sh_size > static_cast<uint64_t>(file_size) - names_header.sh_offset)
            return 0;
        names.resize(static_cast<size_t>(names_header.sh_size) + 1);
        file.seekg(static_cast<std::streamoff>(names_header.sh_offset));
        file.read(names.data(), static_cast<std::streamsize>(names_header.sh_size));
        if (!file)
            return 0;
        names[names_header.sh_size] = 0;
    }
    for (uint16_t index = 0; index < header.e_shnum; ++index) {
        const Elf64_Shdr &section = sections[index];
        const char *name;

        if (section.sh_name >= names.size() - 1)
            continue;
        name = names.data() + section.sh_name;
        if (std::strcmp(name, ".llvm_stackmaps") != 0)
            continue;
        if (section.sh_offset > static_cast<Elf64_Off>(file_size)
            || section.sh_size > static_cast<uint64_t>(file_size) - section.sh_offset)
            return 0;
        section_offset = section.sh_offset;
        section_size = section.sh_size;
        found = true;
        break;
    }
    if (!found || section_size < 16)
        return 0;

    std::vector<uint8_t> section(static_cast<size_t>(section_size));
    file.clear();
    file.seekg(static_cast<std::streamoff>(section_offset));
    file.read(reinterpret_cast<char *>(section.data()), static_cast<std::streamsize>(section.size()));
    if (!file)
        return 0;
    std::vector<Map> maps;
    size_t cursor = 0;
    while (cursor + 16 <= section.size() && section[cursor] == 3) {
        Map map;

        if (!parse_map(section, cursor, &map))
            break;
        maps.push_back(map);
        cursor = map.end;
    }
    if (maps.size() < 2 || !remainder_clear(section, cursor))
        return 0;

    uint32_t functions = 0;
    uint32_t constants = 0;
    uint32_t records = 0;
    for (const Map &map : maps) {
        functions += map.functions;
        constants += map.constants;
        records += map.records;
    }
    std::vector<uint8_t> joined(16, 0);
    joined[0] = 3;
    write_u32(joined, 4, functions);
    write_u32(joined, 8, constants);
    write_u32(joined, 12, records);
    for (const Map &map : maps)
        append_span(joined, section, map.function_at, static_cast<size_t>(map.functions) * 24);
    for (const Map &map : maps)
        append_span(joined, section, map.constant_at, static_cast<size_t>(map.constants) * 8);
    for (const Map &map : maps)
        append_span(joined, section, map.record_at, map.end - map.record_at);
    if (joined.size() > section.size()) {
        std::fputs("scpp: stack map grew during the join\n", stderr);
        return 1;
    }
    joined.resize(section.size(), 0);
    file.clear();
    file.seekp(static_cast<std::streamoff>(section_offset));
    file.write(reinterpret_cast<const char *>(joined.data()), static_cast<std::streamsize>(joined.size()));
    file.flush();
    if (!file) {
        std::fputs("scpp: could not join stack maps\n", stderr);
        return 1;
    }
    return 0;
}
