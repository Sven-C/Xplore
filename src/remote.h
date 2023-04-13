#ifndef REMOTE_H
#define REMOTE_H

#include <stdint.h>
#include <sys/user.h>
#include <sys/types.h>

void do_wait(pid_t pid);
struct user_regs_struct read_regs(pid_t pid);
void write_regs(pid_t pid, struct user_regs_struct regs);
uint8_t read_byte (pid_t pid, unsigned long address);
uint16_t read_short(pid_t pid, unsigned long address);
uint32_t read_int(pid_t pid, unsigned long address);
long read_long(pid_t pid, unsigned long address);
unsigned long read_ulong(pid_t pid, unsigned long address);
void write_long(pid_t pid, void* address, long data);
void write_bytes(pid_t pid, void* address, uint8_t* data, size_t len);

#endif