#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stdint.h>
#include <stddef.h>

void assembler_compile(char *stream, uint32_t **instructions_out, size_t *instructions_size_out);
char *read_file(char *file_path);
#endif // ASSEMBLER_H
