#ifndef ELF_UTILS_H
#define ELF_UTILS_H

#include <elf.h>
#include <stdint.h>
#include <sys/types.h>

#define RELA_MAGIC_VALUE 7

struct dyn_segment_info {
    Elf64_Addr start;
    uint64_t size;
};

struct plt_section_info {
    Elf64_Off offset;
    uint64_t total_size;
    uint64_t entry_kind;
};

struct sym_tab_info {
    Elf64_Off offset;
    uint64_t entry_size;
};

struct str_tab_info {
    Elf64_Off offset;
    uint64_t size;
};

struct dyn_segment_info get_dyn_info(pid_t pid, Elf64_Addr base_addr);
struct plt_section_info* get_plt_section_info(pid_t pid, struct dyn_segment_info dyn_info, struct plt_section_info* result);
struct sym_tab_info* get_sym_tab_info(pid_t pid, struct dyn_segment_info dyn_info, struct sym_tab_info* result);
struct str_tab_info* get_str_tab_info(pid_t pid, struct dyn_segment_info dyn_info, struct str_tab_info* result);

#endif