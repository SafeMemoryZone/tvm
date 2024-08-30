#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint32_t *insts;
  size_t size;
  size_t capacity;
} InstsOut;

int assembler_compile(char *stream_begin, InstsOut *insts_out);
int read_file(char *file_path, char **contents_out);
#endif // ASSEMBLER_H
