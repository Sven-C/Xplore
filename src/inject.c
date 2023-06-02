#define _GNU_SOURCE // To get GNU basename() function.

#include <elf.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "elf_utils.h"
#include "inject.h"
#include "remote.h"

#define INSTRUCTION_OFFSET 0x20
#define PLT_ENTRY_ADDRESS_OFFSET 0x0
#define GOT_ENTRY_ADDRESS_OFFSET 0x8
#define HOOK_ADDRESS_OFFSET 0x10
#define SCRATCH_PAD_OFFSET 0x18

struct hook_function {
    uint8_t* ins;
    size_t size;
};

uint64_t get_base_addr(pid_t pid) {
    char child_mem_maps_filename[50];
    memset(child_mem_maps_filename, '\0', sizeof(child_mem_maps_filename));
    snprintf(child_mem_maps_filename, sizeof(child_mem_maps_filename) - 1, "/proc/%d/maps", pid);
    printf("Opening %s\n", child_mem_maps_filename);
    FILE* mem_maps = fopen(child_mem_maps_filename, "r");
    char line[48];
    size_t expected_count = (sizeof(line) - 1)/sizeof(char);
    size_t read = 0;
    char* end = NULL;
    clearerr(mem_maps);
    do {
        size_t just_read = fread(line, sizeof(char), expected_count, mem_maps);
        if (ferror(mem_maps)) {
            printf("Error: Failed to read %s\n", child_mem_maps_filename);
        }
        if (feof(mem_maps)) {
            printf("Warning: Unexpected EOF while reading %s\n", child_mem_maps_filename);
        }
        read += just_read;
        end = strstr(line, "-");
    } while (read < expected_count && end == NULL);
    fclose(mem_maps);
    if (!end) {
        printf("Warning: '-' not found while parsing base address\n");
    }
    return strtoull(line, &end, 16);
}

void singlestep(pid_t pid) {
    if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
        perror("Ptrace singlestep");
        printf("Error: %s\n", strerror(errno));
    }
    do_wait(pid);
}

uint64_t allocate_remote_memory(pid_t pid) {
    // read current regs and prepare mmap regs
    struct user_regs_struct old_regs = read_regs(pid);
    long old_ins_data = read_long(pid, old_regs.rip);
    struct user_regs_struct mmap_regs;
    memmove(&mmap_regs, &old_regs, sizeof(struct user_regs_struct));
    mmap_regs.rax = SYS_mmap;
    mmap_regs.rdi = 0x0;
    mmap_regs.rsi = PAGE_SIZE;
    mmap_regs.rdx = PROT_READ | PROT_WRITE | PROT_EXEC;
    mmap_regs.r10 = MAP_PRIVATE | MAP_ANONYMOUS;
    mmap_regs.r8 = -1;
    mmap_regs.r9 = 0;
    write_regs(pid, mmap_regs);
    // read and replace current instructions with a syscall
    uint8_t new_ins[8] = {
        0x0f, 0x05, // syscall
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0 // padding
    };
    long new_ins_data;
    memmove(&new_ins_data, new_ins, sizeof(new_ins_data));
    write_long(pid, (void*) old_regs.rip, new_ins_data);

    singlestep(pid);
    struct user_regs_struct mmap_result = read_regs(pid);
    uint64_t remote_address = mmap_result.rax;
    write_regs(pid, old_regs);
    write_long(pid, (void*) old_regs.rip, old_ins_data);
    return remote_address;
}

struct hook_function look_up_hook_for(char* symbol_name, char** hook_filenames, int hook_filenames_count) {
    size_t symbol_name_len = strlen(symbol_name);
    for (int i = 0; i < hook_filenames_count; i++) {
        char* hook_filename = hook_filenames[i];
        char* hooked_symbol = basename(hook_filename);
        if (strncmp(hooked_symbol, symbol_name, symbol_name_len) == 0) {
            struct hook_function result;
            struct stat st;
            stat(hook_filename, &st);
            result.size = st.st_size;
            result.ins = malloc(result.size);
            printf("Loading hook %s (%ld bytes) %s\n", hooked_symbol, st.st_size, hook_filename);
            size_t copied = 0;
            FILE* hook_file = fopen(hook_filename, "r");
            if (!hook_file) {
                printf("Failed to open %s (error %d: %s)\n", hook_filename, errno, strerror(errno));
                exit(1);
            }
            while (!feof(hook_file) && !ferror(hook_file)) {
                copied += fread(result.ins + copied, sizeof(uint8_t), result.size, hook_file);
            }
            if (ferror(hook_file)) {
                printf("Error while reading %s (error %d: %s)\n", hook_filename, ferror(hook_file), strerror(ferror(hook_file)));
            }
            printf("Read %ld bytes from disk\n", copied);
            return result;
        }
    }
    return (struct hook_function) {NULL, 0};
}

void place_hook(pid_t pid, struct hook_function hook_function, uint64_t base_addr, uint64_t scratch_pad_addr, uint64_t got_entry_addr, char* symbol_name) {
    uint64_t hook_base = allocate_remote_memory(pid);
    printf("Allocated remote memory: %p\n", (void*) hook_base);

    unsigned long old_got_pointer_value = read_ulong(pid, got_entry_addr);
    uint64_t plt_entry_addr = base_addr + old_got_pointer_value;
    uint64_t code_start = hook_base + INSTRUCTION_OFFSET;
    printf("PLT entry address: %08lx\n", plt_entry_addr);

    // Patch the global variables which can be used by the hook instructions.
    memcpy(hook_function.ins + PLT_ENTRY_ADDRESS_OFFSET, &plt_entry_addr, sizeof(uint64_t));
    memcpy(hook_function.ins + GOT_ENTRY_ADDRESS_OFFSET, &got_entry_addr, sizeof(uint64_t));
    memcpy(hook_function.ins + HOOK_ADDRESS_OFFSET, &code_start, sizeof(uint64_t));
    memcpy(hook_function.ins + SCRATCH_PAD_OFFSET, &scratch_pad_addr, sizeof(uint64_t));
    
    write_bytes(pid, (void*) hook_base, hook_function.ins, hook_function.size);
    free(hook_function.ins);
    printf("Wrote hook into remote process\n");
    /* New got pointer value will point to our hook
     * located in the memory we allocated after relocations are applied.
     * As the .got section needs to be relocated
     * to support loading of the executable at any address,
     * we substract the base address of the remote process.
     * When the dynamic linker applies the relocations, it will add back
     * the base address, so that the .got entry will point to our hook.
     */
    unsigned long new_got_pointer_value = (unsigned long) (hook_base + INSTRUCTION_OFFSET - base_addr);
    write_long(pid, (void*) got_entry_addr, new_got_pointer_value);
    printf("Pwned GOT @(%p) %08lx -> %08lx\n", (void*) got_entry_addr, old_got_pointer_value, new_got_pointer_value);
}

void patch_got(pid_t pid, struct arguments* arguments, uint64_t base_addr, uint64_t scratch_pad_addr, struct plt_section_info plt, struct str_tab_info str, struct sym_tab_info sym) {
    unsigned long plt_entry_size = (plt.entry_kind == RELA_MAGIC_VALUE ? sizeof(Elf64_Rela) : sizeof(Elf64_Rel));
    unsigned long plt_count = plt.total_size / plt_entry_size;
    for (int i = 0; i < plt_count; i++) {
        Elf64_Addr rel_start = base_addr + plt.offset + i * plt_entry_size;
        unsigned long rel_info = read_ulong(pid, rel_start+sizeof(Elf64_Addr));
        unsigned long symbol_index = ELF64_R_SYM(rel_info);
        uint32_t name_index = read_int(pid, base_addr + sym.offset + symbol_index * sym.entry_size);
        char symbol_name[1024];
        int i = 0;
        uint8_t c = -1;
        do {
             c = read_byte(pid, base_addr + str.offset + name_index + i);
             symbol_name[i] = c;
             i++;
        } while (c != '\0' && i < sizeof(symbol_name));
        if (i == sizeof(symbol_name)) {
            symbol_name[i - 1] = '\0';
            printf("Relocation entry for symbol %s (truncated)\n", symbol_name);
        } else {
            printf("Relocation entry for symbol %s\n", symbol_name);
        }
        unsigned long rel_type = ELF64_R_TYPE(rel_info);
        Elf64_Off rel_offset = read_ulong(pid, rel_start);
        struct hook_function hook_function = look_up_hook_for(symbol_name, arguments->hook_filenames, arguments->hook_count);
        if (hook_function.size > 0) {
            uint64_t got_addr = base_addr + rel_offset;
            place_hook(pid, hook_function, base_addr, scratch_pad_addr, got_addr, symbol_name);
        }
    }
}

void perform_hooks_in_child(pid_t pid, struct arguments* arguments ) {
    do_wait(pid);
    printf("Preparing hooks\n");
    uint64_t base_addr = get_base_addr(pid);
    printf("Base address: %08lx\n", base_addr);
    struct dyn_segment_info dyn_info = get_dyn_info(pid, base_addr);
    struct plt_section_info plt_section_info;
    if (get_plt_section_info(pid, dyn_info, &plt_section_info) == NULL) {
        printf("Failed to find plt section\n");
        exit(1);
    }
    printf("[plt] offset: %lu, size: %lu, entry kind: %lu\n", plt_section_info.offset, plt_section_info.total_size, plt_section_info.entry_kind);
    struct sym_tab_info sym_tab_info;
    if (get_sym_tab_info(pid, dyn_info, &sym_tab_info) == NULL) {
        printf("Failed to find sym tab section\n");
        exit(1);
    }
    printf("[sym tab] offset: %lu, entry size: %lu\n", sym_tab_info.offset, sym_tab_info.entry_size);
    struct str_tab_info str_tab_info;
    if (get_str_tab_info(pid, dyn_info, &str_tab_info) == NULL) {
        printf("Failed to find str tab section\n");
        exit(1);
    }
    printf("[str tab] offset: %lu, size: %lu\n", str_tab_info.offset, str_tab_info.size);
    uint64_t scratch_pad_addr = allocate_remote_memory(pid);
    printf("Allocating shared memory at %p\n", (void*) scratch_pad_addr);
    patch_got(pid, arguments, base_addr, scratch_pad_addr, plt_section_info, str_tab_info, sym_tab_info);
    printf("All done\n");
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    waitpid(pid, NULL, 0);
}