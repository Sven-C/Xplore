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
    size_t buffer_size = st.st_size;
    // This check is needed to properly read pseudo-files located in the /proc/ pseudo-filesystem.
    if (buffer_size == 0) {
        buffer_size = 1024;
    }
    uint8_t* content = malloc(buffer_size);
    buffer.content = content;
    buffer.size = buffer_size;
    printf("Reading file %s (%ld bytes)\n", filename, buffer.size);
    size_t copied = 0;
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open %s (error %d: %s)\n", filename, errno, strerror(errno));
        exit(1);
    }
    // For pseudo-files, e.g. /proc/<pid>/maps, feof(file) will return false, while we have already fully read the buffer.
    while (!feof(file) && !ferror(file) && copied < buffer.size) {
        copied += fread(buffer.content + copied, sizeof(uint8_t), buffer.size - copied, file);
    }
    if (ferror(file)) {
        printf("Error while reading %s (error %d: %s)\n", filename, ferror(file), strerror(ferror(file)));
        exit(1);
    }
    printf("Read %ld bytes from %s\n", copied, filename);
    return buffer;
}