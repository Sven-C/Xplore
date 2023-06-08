#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stddef.h>
#include <stdint.h>

struct buffer {
    uint8_t* content;
    size_t size;
};

struct buffer read_file(char* filename);

#endif