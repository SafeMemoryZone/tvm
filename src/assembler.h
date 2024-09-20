#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stddef.h>

#include "vm.h"

typedef struct {
  inst_ty *insts;
  size_t size;      // size in bytes
  size_t capacity;  // capacity in bytes
} InstsOut;

int assembler_compile(char *filename, char *file_first_char, InstsOut *insts_out);
int read_file(char *file_path, char **contents_out);
int read_file_insts(char *file_path, inst_ty **contents_out, long *insts_count_out);
#endif  // ASSEMBLER_H
