#include <stdlib.h>
#include <stdio.h>

size_t hook_fread(void* buffer, size_t element_size, size_t count, FILE* file) {
    return 0;
}