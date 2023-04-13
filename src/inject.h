#ifndef INJECT_H
#define INJECT_H

#include <elf.h>
#include <stdint.h>
#include <unistd.h>

#include "arguments.h"

void perform_hooks_in_child(pid_t pid, struct arguments* arguments);

#endif
