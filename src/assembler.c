#include "assembler.h"
#include "common.h"
#include "vm.h"
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONSTRUCT_TERNARY_INST(_mnemonic, _op1, _op2, _op3, _flags)            \
  (((_flags) << 17) | ((_op3) << 14) | ((_op2) << 11) | ((_op1) << 8) |        \
   (_mnemonic))

enum TokenType {
  TT_EOF = 0,
  TT_IDENTIFIER,
  TT_NUMERIC_LIT,
  TT_REGISTER,
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

Token get_next_token(char *curr_pos, char *stream_begin) {
  int ln, col;

  // skip whitespace
  while (isspace(*curr_pos) || *curr_pos == ',')
    curr_pos++;

  if (*curr_pos == 0)
    return (Token){.ty = TT_EOF};

  char *identifier_first_char = curr_pos;
  char *identifier_last_char = NULL;

  // identifier
  while (is_identifier(*curr_pos)) {
    identifier_last_char = curr_pos;
    curr_pos++;
  }

  if (identifier_last_char)
    return (Token){.first_char = identifier_first_char,
                   .last_char = identifier_last_char,
                   .ty = TT_IDENTIFIER};

  // register
  size_t remaining_len = strlen(curr_pos);
  if (remaining_len >= 3 && *curr_pos == '#') {
    if (curr_pos[1] != 'R' && curr_pos[1] != 'r') {
      get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
      fatal_err("Assembler err: Expected register number after '#' on %d:%d\n",
                ln, col);
    }

    int reg_num = -1;
    if (curr_pos[2] < '0' || curr_pos[2] > '7') {
      get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
      fatal_err("Assembler err: Invalid register on %d:%d\n", ln, col);
    } else
      reg_num = curr_pos[2] - '0';

    return (Token){.first_char = curr_pos,
                   .last_char = curr_pos + 2,
                   .ty = TT_REGISTER,
                   .int_num = reg_num};
  }

  // number literals
  if ((*curr_pos >= '0' && *curr_pos <= '9') ||
      (*curr_pos == '-' && remaining_len >= 2 &&
       (curr_pos[1] >= '0' && curr_pos[1] <= '9'))) {
    // TODO: float literals
    char *num_end_ptr;
    long long num = strtoll(curr_pos, &num_end_ptr, 0);
    int num_is_unsigned = 0;
    if (num == LONG_MIN || num == LLONG_MAX) {
      if (*curr_pos == '-')
        goto err;
      // try unsigned (bit pattern gets preserved)
      num = strtoull(curr_pos, &num_end_ptr, 0);
      num_is_unsigned = 1;
      if ((unsigned long long)num == ULLONG_MAX) {
        get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
        fatal_err("Assembler err: Number literal overflow on %d:%d\n", ln, col);
      } else
        goto finish;
    }

  err:
    if (num == LLONG_MIN || num == ULLONG_MAX) {
      get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
      fatal_err("Assembler err: Number literal underflow on %d:%d\n", ln, col);
    } else if (num == LLONG_MAX) {
      get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
      fatal_err("Assembler err: Number literal overflow on %d:%d\n", ln, col);
    }
  finish:
    return (Token){.first_char = curr_pos,
                   .last_char = num_end_ptr - 1,
                   .ty = TT_NUMERIC_LIT,
                   .int_num = num,
                   .num_is_float = 0,
                   .num_is_unsigned = num_is_unsigned};
  }

  get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
  fatal_err("Assembler err: Unknown token '%c' on %d:%d\n", *curr_pos, ln, col);
  return (Token){};
}

int cmp_mnemonic(char *mnemonic, char *tok_first_char, char *tok_last_char, int allowed_flags) {
  int n = tok_last_char - tok_first_char + 1;
  int len = strlen(mnemonic);
  if (n != len)
    return -1;

  int i = 0;
  for (; i < len; i++) {
    if (tolower(mnemonic[i]) != tolower(tok_first_char[i]))
      return -1;
  }

  // flags
  int flags = 0;

  if(i < n) {
    if(tok_first_char[i] == 'f') {
      flags |= FLAG_FLOAT;
      i++;
    }
    else if(tok_first_char[i] == 'u') {
      flags |= FLAG_UNSIGNED;
      i++;
    }

    int remaining_len = n - i;

    if(!remaining_len)
      goto return_flags;

    if(remaining_len == 2) {
      if(tok_first_char[i] == '3' && tok_first_char[i + 1] == '2') {
        flags |= FLAG_32;
        goto return_flags;
      }
      else if(tok_first_char[i] == '6' && tok_first_char[i + 1] == '4') {
        flags |= FLAG_64;
        goto return_flags;
      }

      return -1;
    }
    else return -1;
  }

return_flags:
  if(flags & ~allowed_flags)
    return -1;

  return flags;
}

void insts_out_append_data(InstsOut *out, void *data, size_t data_size) {
  if (out->size + data_size > out->capacity) {
    size_t new_capacity = (out->size + data_size) * 2;
    out->insts = realloc(out->insts, new_capacity * sizeof(uint32_t));
    out->capacity = new_capacity;
  }

  memcpy(((uint8_t *)out->insts) + out->size, data, data_size);
  out->size += data_size;
}

void insts_out_append(InstsOut *out, uint32_t inst) {
  insts_out_append_data(out, &inst, sizeof(uint32_t));
}


Token expect_next_tok(char *curr_pos, char *stream_begin, int expected_tok_ty,
    Token *curr_tok_out, char *msg, ...) {
  *curr_tok_out = get_next_token(curr_pos, stream_begin);

  if (curr_tok_out->ty != expected_tok_ty) {
    va_list args_ptr;
    va_start(args_ptr, msg);
    vfprintf(stderr, msg, args_ptr);
    int ln, col;
    get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
    fprintf(stderr, " on line %d:%d\n", ln, col);
    va_end(args_ptr);
  }

  return *curr_tok_out;
}

InstsOut assembler_compile(char *stream_begin) {
  char *curr_pos = stream_begin;
  Token curr_tok = get_next_token(curr_pos, stream_begin);
  InstsOut insts_out = {.insts = NULL, .size = 0, .capacity = 0};

  while (curr_tok.ty != TT_EOF) {
    int ln, col;
    if (curr_tok.ty != TT_IDENTIFIER) {
      get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
      fatal_err("Assembler err: Expected an instruction on %d:%d\n", ln, col);
    }

    int flags = 0;
    if((flags = cmp_mnemonic("halt", curr_tok.first_char, curr_tok.last_char, 0)) != -1) {
      insts_out_append(&insts_out, MNEMONIC_HALT);  
    }
    else if((flags = cmp_mnemonic("add", curr_tok.first_char, curr_tok.last_char, FLAG_UNSIGNED | FLAG_FLOAT)) != -1) {
      Token op1 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op2 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op3 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      insts_out_append(&insts_out, CONSTRUCT_TERNARY_INST(MNEMONIC_ADD, op1.int_num, op2.int_num, op3.int_num, flags));
    }
    else if((flags = cmp_mnemonic("sub", curr_tok.first_char, curr_tok.last_char, FLAG_UNSIGNED | FLAG_FLOAT)) != -1) {
      Token op1 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op2 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op3 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      insts_out_append(&insts_out, CONSTRUCT_TERNARY_INST(MNEMONIC_SUB, op1.int_num, op2.int_num, op3.int_num, flags));
    }
    else if((flags = cmp_mnemonic("mul", curr_tok.first_char, curr_tok.last_char, FLAG_UNSIGNED | FLAG_FLOAT)) != -1) {
      Token op1 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op2 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op3 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      insts_out_append(&insts_out, CONSTRUCT_TERNARY_INST(MNEMONIC_MUL, op1.int_num, op2.int_num, op3.int_num, flags));
    }
    else if((flags = cmp_mnemonic("div", curr_tok.first_char, curr_tok.last_char, FLAG_UNSIGNED | FLAG_FLOAT)) != -1) {
      Token op1 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op2 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op3 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      insts_out_append(&insts_out, CONSTRUCT_TERNARY_INST(MNEMONIC_DIV, op1.int_num, op2.int_num, op3.int_num, flags));
    }
    else if((flags = cmp_mnemonic("loadc", curr_tok.first_char, curr_tok.last_char, FLAG_UNSIGNED | FLAG_FLOAT | FLAG_32 | FLAG_64)) != -1) {
      Token op1 = expect_next_tok(curr_pos, stream_begin, TT_REGISTER, &curr_tok, "Expected register");
      Token op2 = expect_next_tok(curr_pos, stream_begin, TT_NUMERIC_LIT, &curr_tok, "Expected a number");

      insts_out_append(&insts_out, CONSTRUCT_TERNARY_INST(MNEMONIC_LOAD_CONST, op1.int_num, 0, 0, flags));

      if (flags & FLAG_FLOAT) {
        if (flags & FLAG_32) {
          float value_32 = (float)op2.float_num;
          insts_out_append_data(&insts_out, &value_32, sizeof(value_32));
        } else if (flags & FLAG_64) {
          double value_64 = op2.float_num;
          insts_out_append_data(&insts_out, &value_64, sizeof(value_64));
        } else {
          get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
          fatal_err("Assembler err: No valid 32bit or 64bit flag set for floating-point number on %d:%d\n", ln, col);
        }
      } else {
        if (flags & FLAG_32) {
          uint32_t value_32 = (uint32_t)op2.int_num;
          insts_out_append_data(&insts_out, &value_32, sizeof(value_32));
        } else if (flags & FLAG_64) {
          uint64_t value_64 = (uint64_t)op2.int_num;
          insts_out_append_data(&insts_out, &value_64, sizeof(value_64));
        } else {
          get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
          fatal_err("Assembler err: No valid 32bit or 64bit flag set for integer number on %d:%d\n", ln, col);
        }
      }
    }

    else {
      get_parse_ptr_location(curr_pos, stream_begin, &ln, &col);
      int n = curr_tok.last_char - curr_tok.first_char + 1;
      fatal_err("Assembler err: Unknow instruction '%*.s' on %d:%d\n", n,
          curr_tok.first_char, ln, col);
    }

    curr_pos = curr_tok.last_char + 1;
    curr_tok = get_next_token(curr_pos, stream_begin);
  }

  return insts_out;
}

char *read_file(char *file_path) {
  int fd = open(file_path, O_RDONLY);
  if (fd < 0)
    fatal_err("Assembler err: Could not open file '%s'\n", file_path);

  struct stat st;
  if (fstat(fd, &st) < 0) {
    close(fd);
    fatal_err("Assembler err: Could not obtain file size for '%s'\n",
        file_path);
  }

  off_t filesize = st.st_size;
  char *buffer = malloc(filesize + 1);

  ssize_t bytes_read = read(fd, buffer, filesize);
  if (bytes_read < 0) {
    free(buffer);
    close(fd);
    fatal_err("Assembler err: Could not read file '%s'\n", file_path);
  }

  buffer[filesize] = '\0';

  close(fd);
  return buffer;
}
