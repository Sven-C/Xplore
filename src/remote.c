#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "remote.h"

long poke(pid_t pid, void* address) {
    errno = 0;
    long word = ptrace(PTRACE_PEEKTEXT, pid, address, NULL);
    if (errno) {
        printf("Failed to read a word from remote process\n");
        printf("Address: %p\n", address);
        exit(-1);
    }
    return word;
}

void do_wait(pid_t pid) {
    int status = 0;
    while (!WIFSTOPPED(status)) {
        if (waitpid(pid, &status, 0) == -1) {
            printf("Error: wait on child %d: %s\n", pid, strerror(errno));
            exit(-1);
        }
    }
}

struct user_regs_struct read_regs(pid_t pid) {
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
    {
        printf("Failed to read registers from remote process: %s\n", strerror(errno));
        exit(-1);
    }
    return regs;
}

void write_regs(pid_t pid, struct user_regs_struct regs) {
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        printf("Failed to write registers to remote process: %s\n", strerror(errno));
        exit(-1);
    }
}

uint8_t read_byte(pid_t pid, unsigned long address) {
    return (uint8_t) (poke(pid, (void*) address) & 0xff);
}

uint16_t read_short(pid_t pid, unsigned long address) {
    return (uint16_t) (poke(pid, (void*) address) & 0xffff);
}

uint32_t read_int(pid_t pid, unsigned long address) {
    return (uint32_t) (poke(pid, (void*) address) & 0xffffffff);
}

long read_long(pid_t pid, unsigned long address) {
    return poke(pid, (void*) address);
}

unsigned long read_ulong(pid_t pid, unsigned long address) {
    return (unsigned long) poke(pid, (void*) address);
}

void write_long(pid_t pid, void* address, long data) {
    ptrace(PTRACE_POKETEXT, pid, address, (void*) data);
}

void write_bytes(pid_t pid, void* address, uint8_t* data, size_t len) {
    long poke_data;
    int allocated_memory = 0;
    uint8_t* prepared_data = data;
    size_t poke_size = sizeof(poke_data);
    int remainder = len % poke_size;
    if (remainder != 0) {
        allocated_memory = 1;
        prepared_data = malloc(len + poke_size - remainder);
        memcpy(prepared_data, data, len);
        memset(prepared_data + len, '\0', poke_size - remainder);
    }
    for (int i = 0; i < len; i += poke_size) {
        memmove(&poke_data, prepared_data + i, poke_size);
        write_long(pid, address + i, poke_data);
    }
    if (allocated_memory) {
        free(prepared_data);
    }
}