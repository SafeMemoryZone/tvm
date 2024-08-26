#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint32_t *insts;
  size_t size;
  size_t capacity;
} InstsOut;

InstsOut assembler_compile(char *stream_begin);
char *read_file(char *file_path);
#endif // ASSEMBLER_H
