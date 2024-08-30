#include "assembler.h"
#include "common.h"
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
  TT_EOF = 0,
  TT_IDENT,
  TT_NUM,
  TT_REGISTER,
};

typedef struct {
  char *first_char;
  char *last_char;
  int ty;
  bool num_is_f64;
  bool num_is_u64;
  union {
    double f64;
    uint64_t i64;
    uint64_t u64;
  };
} Token;

void get_curr_pos_loc(char *curr_pos, char *stream_begin, int *line_num, int *col_num) {
  int ln = 1;
  int col = 1;

  for (char *it = stream_begin; it < curr_pos; it++) {
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

void print_syntax_err(char *err_loc, char *err_loc_end, char *stream_begin, char *fmt, ...) {
    fprintf(stderr, "Assembler error: ");
    va_list args_ptr;
    va_start(args_ptr, fmt);
    vfprintf(stderr, fmt, args_ptr);
    va_end(args_ptr);

    int ln, col;
    get_curr_pos_loc(err_loc, stream_begin, &ln, &col);
    fprintf(stderr, " (line %d:%d)\n", ln, col);

    char *ln_start_ptr = err_loc;
    while (ln_start_ptr > stream_begin && *(ln_start_ptr - 1) != '\n')
        ln_start_ptr--;

    char *ln_end_ptr = ln_start_ptr;
    while (*ln_end_ptr != '\n' && *ln_end_ptr != '\0')
        ln_end_ptr++;

    fprintf(stderr, "%.*s\n", (int)(ln_end_ptr - ln_start_ptr), ln_start_ptr);

    for (char *p = ln_start_ptr; p < err_loc; p++) {
        fputc((*p == '\t') ? '\t' : ' ', stderr);
    }

    fputc('^', stderr);
    if (err_loc_end != NULL) {
        for (char *p = err_loc + 1; p < err_loc_end; p++)
            fputc('~', stderr);
    }
    fprintf(stderr, "\n");
}

inline bool is_ident(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int get_next_tok(char *curr_pos, char *stream_begin, Token *tok_out) {
  while(isspace(*curr_pos))
    curr_pos++;

  if(*curr_pos == 0)
    return ERR_CODE_EOF;

  char *ident_first_char = curr_pos;
  char *ident_last_char = NULL;

  while(is_ident(*curr_pos)) {
    ident_last_char = curr_pos;
    curr_pos++;
  }

  if(ident_last_char) {
    *tok_out = (Token) {
      .first_char = ident_first_char,
      .last_char = ident_last_char,
      .ty = TT_IDENT
    };
    return ERR_CODE_OK;
  }

  if(isdigit(*curr_pos)) {
    // TODO: unsigned nums and floats
    char *num_end = NULL;
    int64_t parsed_num = strtoll(curr_pos, &num_end, 0); 

    if(parsed_num == LLONG_MAX) {
      while(isdigit(*curr_pos)) {
        num_end = curr_pos;
        curr_pos++;
      }
      print_syntax_err(curr_pos, num_end, stream_begin, "Number overflow");
      return ERR_CODE_OVERFLOW;
    }
    else if(parsed_num == LLONG_MIN) {
      while(isdigit(*curr_pos)) {
        num_end = curr_pos;
        curr_pos++;
      }
      print_syntax_err(curr_pos, num_end, stream_begin, "Number underflow");
      return ERR_CODE_UNDERFLOW;     
    }

    *tok_out = (Token) {
      .first_char = curr_pos,
      .last_char = num_end - 1,
      .ty = TT_NUM,
      .i64 = parsed_num
    };

    return ERR_CODE_OK;
  }

  return ERR_CODE_UNKNOW_TOK;
}

void insts_out_append_data(InstsOut *out, void *data, size_t data_size) {
  if (out->size + data_size > out->capacity) {
    size_t new_capacity = (out->size + data_size) * 2;
    out->insts = realloc(out->insts, new_capacity);
    out->capacity = new_capacity;
  }

  memcpy(((uint8_t *)out->insts) + out->size, data, data_size);
  out->size += data_size;
}

void insts_out_append(InstsOut *out, uint32_t inst) {
  insts_out_append_data(out, &inst, sizeof(uint32_t));
}

int assembler_compile(char *stream_begin, InstsOut *insts_out) {
  // TODO: implement
  InstsOut insts = {0};
  *insts_out = insts;
  return ERR_CODE_OK;
}

int read_file(char *file_path, char **contents_out) {
  FILE *file = fopen(file_path, "rb");

  if (!file) {
    print_err("File error: Could not open file '%s'", file_path);
    return ERR_CODE_FS;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    print_err("File error: Could not obtain file size for '%s'", file_path);
    return ERR_CODE_FS;
  }

  long filesize = ftell(file);
  if (filesize < 0) {
    fclose(file);
    print_err("File error: Could not determine the file size for '%s'", file_path);
    return ERR_CODE_FS;
  }

  rewind(file);

  char *buffer = malloc(filesize + 1);
  size_t read_size = fread(buffer, 1, filesize, file);

  if (read_size != (unsigned long) filesize) {
    free(buffer);
    fclose(file);
    print_err("File error: Could not read file '%s'", file_path);
    return ERR_CODE_FS;
  }

  buffer[filesize] = '\0';
  fclose(file);
  *contents_out = buffer;

  return ERR_CODE_OK;
}
