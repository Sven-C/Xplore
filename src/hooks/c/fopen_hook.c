#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

FILE* hook_fopen(char* filename, char* mode) {
    FILE* result = fopen(filename, mode);
    int *fd = (int*) (((uint8_t*) result) + 0x70);
    printf("fopen(%s, %s) = %p (open file %p with fd %d)\n", filename, mode, result, result, *fd);
    return result;
}