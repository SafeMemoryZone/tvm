#include "assembler.h"
#include "common.h"
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define LEXICAL_FORMAT_CAPACITY 5

struct InstructionSpec {
  char *mnemonic;
  int lexical_format[LEXICAL_FORMAT_CAPACITY];
  int lexical_format_len;
  void(*optional_encode_func)(uint32_t*, int*);
};

enum TokenType {
  TT_EOF = 0,
  TT_IDENTIFIER,
  TT_NUMERIC_LIT,
  TT_REGISTER,
  TT_COMMA,
};

typedef struct {
  char *first_char;
  char *last_char;
  int ty;
  int num_is_float;
  int num_is_unsigned;
  union {
    double float_num;
    uint64_t int_num;
  };
} Token;

void get_parse_ptr_location(char *ptr, char *stream_begin, int *line_num,
                            int *col_num) {
  int ln = 1;
  int col = 1;

  for (char *it = stream_begin; it < ptr; it++) {
    if (*it == '\n') {
      ln++;
      col = 1;
    } else {
      col++;
    }
  }
  *line_num = ln;
  *col_num = col;
}

static inline int is_identifier(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

Token get_next_token(char *pos, char *stream_begin) {
  int ln, col; // for error handling
  // skip whitespace
  while (isspace(*pos))
    pos++;

  if (*pos == 0)
    return (Token){.ty = TT_EOF};

  if (*pos == ',')
    return (Token){.first_char = pos, .last_char = pos, .ty = TT_COMMA};

  char *identifier_start = pos;
  char *identifier_end = NULL;

  // identifier
  while (is_identifier(*pos)) {
    if (!identifier_end)
      identifier_end = pos;
    else
      identifier_end++;
    pos++;
  }

  if (identifier_end)
    return (Token){.first_char = identifier_start,
                   .last_char = identifier_end,
                   .ty = TT_IDENTIFIER};

  // register
  size_t len = strlen(pos);
  if (len >= 3 && *pos == '#') {
    if (pos[1] != 'R' && pos[1] != 'r') {
      get_parse_ptr_location(pos, stream_begin, &ln, &col);
      fatal_error(
          "Assembler error: Expected register number after '#' on %d:%d\n", ln,
          col);
    }

    int reg_num = -1;
    if (pos[2] < '0' || pos[2] > '7') {
      get_parse_ptr_location(pos, stream_begin, &ln, &col);
      fatal_error("Assembler error: Invalid register on %d:%d\n", ln, col);
    } else
      reg_num = pos[2] - '0';

    return (Token){.first_char = pos,
                   .last_char = pos + 2,
                   .ty = TT_REGISTER,
                   .int_num = reg_num};
  }

  // number literals
  if ((*pos >= '0' && *pos <= '9') ||
      (*pos == '-' && len >= 2 && (pos[1] >= '0' && pos[1] <= '9'))) {
    // TODO: float literals
    char *end_ptr;
    long long num = strtoll(pos, &end_ptr, 0);
    int is_unsigned = 0;
    if (num == LONG_MIN || num == LLONG_MAX) {
      if (*pos == '-')
        goto error;
      // try unsigned (bit pattern gets preserved)
      num = strtoull(pos, &end_ptr, 0);
      is_unsigned = 1;
    }

  error:
    if (num == LLONG_MIN || num == ULLONG_MAX) {
      get_parse_ptr_location(pos, stream_begin, &ln, &col);
      fatal_error("Assembler error: Number literal underflow on %d:%d\n", ln,
                  col);
    } else if (num == LLONG_MAX) {
      get_parse_ptr_location(pos, stream_begin, &ln, &col);
      fatal_error("Assembler error: Number literal overflow on %d:%d\n", ln,
                  col);
    }
    return (Token){.first_char = pos,
                   .last_char = end_ptr - 1,
                   .ty = TT_NUMERIC_LIT,
                   .int_num = num,
                   .num_is_float = 0,
                   .num_is_unsigned = is_unsigned};
  }

  get_parse_ptr_location(pos, stream_begin, &ln, &col);
  fatal_error("Assembler error: Unknown token '%c' on %d:%d\n", *pos, ln, col);
  return (Token){};
}

void assembler_compile(char *stream, uint32_t **instructions_out,
                       size_t *instructions_size_out) {
  char *pos = stream;
  Token tok = get_next_token(pos, stream);
  size_t instructions_size = 0;
  uint32_t *instructions = NULL;

  while (tok.ty != TT_EOF) {
    int n = tok.last_char - tok.first_char + 1;

    // TODO: implement main part of compiler

    pos = tok.last_char + 1;
    tok = get_next_token(pos, stream);
  }

  *instructions_size_out = instructions_size;
  *instructions_out = instructions;
}

char *read_file(char *file_path) {
  int fd = open(file_path, O_RDONLY);
  if (fd < 0)
    fatal_error("Assembler error: Could not open file '%s'\n", file_path);

  struct stat st;
  if (fstat(fd, &st) < 0) {
    close(fd);
    fatal_error("Assembler error: Could not obtain file size for '%s'\n",
                file_path);
  }

  off_t filesize = st.st_size;
  char *buffer = malloc(filesize + 1);

  ssize_t bytes_read = read(fd, buffer, filesize);
  if (bytes_read < 0) {
    free(buffer);
    close(fd);
    fatal_error("Assembler error: Could not read file '%s'\n", file_path);
  }

  buffer[filesize] = '\0';

  close(fd);
  return buffer;
}
