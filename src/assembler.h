#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include "common.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
  inst_ty *insts;
  size_t size; // size in bytes
  size_t capacity; // capacity in bytes
} InstsOut;

int assembler_compile(char *stream_begin, InstsOut *insts_out);
int read_file(char *file_path, char **contents_out);
int read_file_insts(char *file_path, inst_ty **contents_out, long *insts_count_out);
#endif // ASSEMBLER_H
