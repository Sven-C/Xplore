#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <argp.h>

struct arguments
{
  char* executable;
  int silent;
  int verbose;
  char** hook_filenames;
  int hook_count;
};

error_t parse_args(int argc, char** argv, struct arguments* arguments, int* arg_index);

#endif