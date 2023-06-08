#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "io_utils.h"

struct buffer read_file(char* filename) {
    struct buffer buffer;
    struct stat st;
    stat(filename, &st);
    uint8_t* content = malloc(st.st_size);
    buffer.content = content;
    buffer.size = st.st_size;
    printf("Reading file %s (%ld bytes)\n", filename, buffer.size);
    size_t copied = 0;
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open %s (error %d: %s)\n", filename, errno, strerror(errno));
        exit(1);
    }
    while (!feof(file) && !ferror(file)) {
        copied += fread(buffer.content + copied, sizeof(uint8_t), buffer.size, file);
    }
    if (ferror(file)) {
        printf("Error while reading %s (error %d: %s)\n", filename, ferror(file), strerror(ferror(file)));
    }
    printf("Read %ld bytes from %s\n", copied, filename);
    return buffer;
}