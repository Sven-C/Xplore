#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "elf_utils.h"
#include "remote.h"

struct opt_ulong{
    int tag;
    unsigned long value;
};

struct opt_ulong look_up_dynamic_segment(pid_t pid, struct dyn_segment_info dyn_info, unsigned long tag) {
    unsigned long count = dyn_info.size / sizeof(Elf64_Dyn);
    for (unsigned long i = 0; i < count; i++) {
        Elf64_Addr dyn_start = dyn_info.start + i * sizeof(Elf64_Dyn);
        if (read_ulong(pid, dyn_start) == tag) {
            unsigned long value = read_ulong(pid, dyn_start + sizeof(Elf64_Xword));
            struct opt_ulong result = {1, value};
            return result;
        }
    }
    struct opt_ulong result = {0, -1};
    return result;
}

struct dyn_segment_info get_dyn_info(pid_t pid, Elf64_Addr base_addr) {
    Elf64_Off e_phoff = read_ulong(pid, base_addr + 0x20);
    Elf64_Half e_phentsize = read_short(pid, base_addr + 0x36);
    Elf64_Half e_phnum = read_short(pid, base_addr + 0x38);
    for (int i = 0; i < e_phnum; i++) {
        Elf64_Addr e_phstart = base_addr + e_phoff + i * e_phentsize;
        if (read_int(pid, e_phstart) == PT_DYNAMIC) {
            Elf64_Off p_offset = read_ulong(pid, e_phstart + 0x10);
            uint64_t p_memsz = read_ulong(pid, e_phstart + 0x28);
            Elf64_Addr dynamic_segment_start_address = base_addr + p_offset;
            return (struct dyn_segment_info) {dynamic_segment_start_address, p_memsz};
        }
    }
    return (struct dyn_segment_info) {0, 0};
}

struct plt_section_info* get_plt_section_info(pid_t pid, struct dyn_segment_info dyn_info, struct plt_section_info* result) {
    struct opt_ulong opt_jmprel_offset = look_up_dynamic_segment(pid, dyn_info, DT_JMPREL);
    if (opt_jmprel_offset.tag == 0) {
        printf("Failed to find DT_JMPREL\n");
        return NULL;
    }
    Elf64_Addr jmprel_offset = opt_jmprel_offset.value;
    struct opt_ulong opt_jmprel_entries_size = look_up_dynamic_segment(pid, dyn_info, DT_PLTRELSZ);
    if (opt_jmprel_entries_size.tag == 0) {
        printf("Failed to find DT_PLTRELSZ\n");
        return NULL;
    }
    uint64_t jmprel_entries_size = opt_jmprel_entries_size.value;
    struct opt_ulong opt_jmprel_entries_kind = look_up_dynamic_segment(pid, dyn_info, DT_PLTREL);
    if (opt_jmprel_entries_kind.tag == 0) {
        printf("Failed to find DT_PLTREL\n");
        return NULL;
    }
    uint64_t jmprel_entries_kind = opt_jmprel_entries_kind.value;
    result->offset = jmprel_offset;
    result->total_size = jmprel_entries_size;
    result->entry_kind = jmprel_entries_kind;
    return result;
}

struct sym_tab_info* get_sym_tab_info(pid_t pid, struct dyn_segment_info dyn_info, struct sym_tab_info* result) {
    struct opt_ulong opt_symtab = look_up_dynamic_segment(pid, dyn_info, DT_SYMTAB);
    if (opt_symtab.tag == 0) {
        printf("Failed to find DT_SYMTAB\n");
        return NULL;
    }
    unsigned long symtab = opt_symtab.value;
    struct opt_ulong opt_syment = look_up_dynamic_segment(pid, dyn_info, DT_SYMENT);
    if (opt_syment.tag == 0) {
        printf("Failed to find DT_SYMENT\n");
        return NULL;
    }
    unsigned long syment = opt_syment.value;
    result->offset = symtab;
    result->entry_size = syment;
    return result;
}

struct str_tab_info* get_str_tab_info(pid_t pid, struct dyn_segment_info dyn_info, struct str_tab_info* result) {
    struct opt_ulong opt_strtab = look_up_dynamic_segment(pid, dyn_info, DT_STRTAB);
    if (opt_strtab.tag == 0) {
        printf("Failed to find DT_STRTAB\n");
        return NULL;
    }
    unsigned long strtab = opt_strtab.value;
    struct opt_ulong opt_strsz = look_up_dynamic_segment(pid, dyn_info, DT_STRSZ);
    if (opt_strsz.tag == 0) {
        printf("Failed to find DT_STRSZ\n");
        return NULL;
    }
    unsigned long strsz = opt_strsz.value;
    result->offset = strtab;
    result->size = strsz;
    return result;
}