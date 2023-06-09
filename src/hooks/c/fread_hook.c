#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

size_t hook_fread(void* buffer, size_t element_size, size_t count, FILE* file) {
    size_t bytes_read = fread(buffer, element_size, count, file);
    int *fd = (int*) (((uint8_t*) file) + 0x70);
    printf("fread(%p, %lu, %lu, %p) = %lu (read %lu bytes from file with fd %d\n", buffer, element_size, count, file, bytes_read, bytes_read, *fd);
    return bytes_read;
}