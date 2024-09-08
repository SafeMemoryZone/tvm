#include "assembler.h"
#include "error.h"
#include "vm.h"
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYNTAX_ERR_IF(_ctx, _cond, _err_loc_first_char, _err_loc_last_char,    \
                      _fmt, ...)                                               \
  do {                                                                         \
    if (_cond) {                                                               \
      print_syntax_err(_ctx, _err_loc_first_char, _err_loc_last_char, _fmt,    \
                       ##__VA_ARGS__);                                         \
      return RET_CODE_ERR;                                                     \
    }                                                                          \
  } while (0)

#define MISSING_TOK_ERR_IF(_ctx, _out_tok, _tok_err, _fmt, ...)                 \
  do {                                                                         \
    int ret_code;                                                              \
    if ((ret_code = get_next_tok(_ctx, &_out_tok)) != 0) {                     \
      SYNTAX_ERR_IF(_ctx, ret_code == RET_CODE_NORET, _tok_err.last_char,      \
                    _tok_err.last_char, _fmt, ##__VA_ARGS__);                   \
      return ret_code;                                                         \
    }                                                                          \
  } while (0)

enum TokenType {
  TT_IDENT,
  TT_NUM,
  TT_REGISTER,
};

typedef struct {
  char *first_char;
  char *last_char;
  int ty;
  bool num_is_f64;
  union {
    double f64;
    uint64_t i64;
  };
} Token;

typedef struct {
  char *curr_pos;
  char *stream_begin;
  char *filename;
  InstsOut *insts_out;
} CompileCtx;

void get_curr_pos_loc(char *curr_pos, char *stream_begin, int *line_num,
                      int *col_num) {
  int ln = 1;
  int col = 1;

  for (char *it = stream_begin; it < curr_pos; it++) {
    if (*it == '\n') {
      ln++;
      col = 1;
    } else
      col++;
  }
  *line_num = ln;
  *col_num = col;
}

void print_syntax_err(CompileCtx *ctx, char *err_loc_first_char,
                      char *err_loc_last_char, char *fmt, ...) {
  fprintf(stderr, "Assembler error: ");
  va_list args_ptr;
  va_start(args_ptr, fmt);
  vfprintf(stderr, fmt, args_ptr);
  va_end(args_ptr);
  fputc('\n', stderr);

  char *first_ln_char_ptr = err_loc_first_char;
  while (first_ln_char_ptr > ctx->stream_begin && first_ln_char_ptr[-1] != '\n')
    first_ln_char_ptr--;

  char *ln_end_ptr = first_ln_char_ptr;
  while (*ln_end_ptr != '\n' && *ln_end_ptr != '\0')
    ln_end_ptr++;

  int ln, col;
  get_curr_pos_loc(err_loc_last_char, ctx->stream_begin, &ln, &col);
  int off = fprintf(stderr, "%s %d:%d: ", ctx->filename, ln, col);
  fprintf(stderr, "%.*s\n", (int)(ln_end_ptr - first_ln_char_ptr),
          first_ln_char_ptr);

  for (int i = 0; i < off; i++)
    fputc(' ', stderr);

  for (char *p = first_ln_char_ptr; p < err_loc_first_char; p++)
    fputc((*p == '\t') ? '\t' : ' ', stderr);
  fputc('^', stderr);
  for (char *p = err_loc_first_char + 1; p <= err_loc_last_char; p++)
    fputc('~', stderr);
  fprintf(stderr, "\n");
}

bool is_ident(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_hex(char c) {
  return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

char *extend_num_err(char *curr_pos) {
  bool allow_hex = false;

  if (*curr_pos == '-')
    curr_pos++;

  if (*curr_pos == '0' && (curr_pos[1] == 'x' || curr_pos[1] == 'X')) {
    allow_hex = true;
    curr_pos += 2;
  }

  while (isdigit(*curr_pos) || (allow_hex && is_hex(*curr_pos)))
    curr_pos++;

  return curr_pos - 1;
}

int get_next_tok(CompileCtx *ctx, Token *tok_out) {
  while (isspace(*ctx->curr_pos))
    ctx->curr_pos++;

  if (!*ctx->curr_pos)
    return RET_CODE_NORET;

  char *ident_first_char = ctx->curr_pos;
  char *ident_last_char = NULL;

  while (is_ident(*ctx->curr_pos)) {
    ident_last_char = ctx->curr_pos;
    ctx->curr_pos++;
  }

  if (ident_last_char) {
    *tok_out = (Token){.first_char = ident_first_char,
                       .last_char = ident_last_char,
                       .ty = TT_IDENT};
    ctx->curr_pos = ident_last_char + 1;
    return RET_CODE_OK;
  }

  if (isdigit(*ctx->curr_pos) ||
      (*ctx->curr_pos == '-' && isdigit(ctx->curr_pos[1]))) {
    char *num_end = NULL;
    int64_t parsed_num = strtoll(ctx->curr_pos, &num_end, 0);

    SYNTAX_ERR_IF(ctx, num_end != 0 && !isspace(*num_end), num_end, num_end,
                  "Expected whitespace after number");

    if (parsed_num == LLONG_MAX) {
      print_syntax_err(ctx, ctx->curr_pos, extend_num_err(ctx->curr_pos),
                       "Number overflow");
      return RET_CODE_ERR;
    } else if (parsed_num == LLONG_MIN) {
      print_syntax_err(ctx, ctx->curr_pos, extend_num_err(ctx->curr_pos),
                       "Number underflow");
      return RET_CODE_ERR;
    }

    *tok_out = (Token){.first_char = ctx->curr_pos,
                       .last_char = num_end - 1,
                       .ty = TT_NUM,
                       .i64 = parsed_num};

    ctx->curr_pos = num_end;
    return RET_CODE_OK;
  }

  print_syntax_err(ctx, ctx->curr_pos, ctx->curr_pos, "Unknown token");
  return RET_CODE_ERR;
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

void insts_out_append(InstsOut *out, inst_ty inst) {
  insts_out_append_data(out, &inst, sizeof(inst_ty));
}

bool cmp_mnemonic(char *mnemonic, char *first_tok_char) {
  if (strlen(first_tok_char) < strlen(mnemonic))
    return false;

  for (unsigned int i = 0; i < strlen(mnemonic); i++) {
    if (mnemonic[i] != first_tok_char[i])
      return false;
  }

  return true;
}

int compile_inst(CompileCtx *ctx) {
  Token inst;
  int ret_code;

  if ((ret_code = get_next_tok(ctx, &inst)) != 0)
    return ret_code;

  if (inst.ty != TT_IDENT)
    goto unknown_inst;

  if (cmp_mnemonic("exit", inst.first_char)) {
    Token exit_ret_code;

    MISSING_TOK_ERR_IF(ctx, exit_ret_code, inst,
                       "Expected an exit code after 'exit'");
    SYNTAX_ERR_IF(ctx, exit_ret_code.ty != TT_NUM, exit_ret_code.first_char,
                  exit_ret_code.last_char,
                  "Expected an exit code after 'exit'");
    SYNTAX_ERR_IF(ctx, exit_ret_code.i64 < 0 || exit_ret_code.i64 > 255,
                  exit_ret_code.first_char, exit_ret_code.last_char,
                  "Exit code has to be between 0 and 255");

    insts_out_append(ctx->insts_out, exit_ret_code.i64 << 8 | MNEMONIC_EXIT);
    return RET_CODE_OK;
  }
  // TODO: implement insts
  else if (cmp_mnemonic("add", inst.first_char)) {
  } else if (cmp_mnemonic("sub", inst.first_char)) {
  } else if (cmp_mnemonic("mul", inst.first_char)) {
  } else if (cmp_mnemonic("div", inst.first_char)) {
  }

unknown_inst:
  print_syntax_err(ctx, inst.first_char, inst.last_char,
                   "Unknown instruction '%.*s'",
                   inst.last_char - inst.first_char + 1, inst.first_char);
  return RET_CODE_ERR;
}

int assembler_compile(char *filename, char *stream_begin, InstsOut *insts_out) {
  InstsOut insts = {0};
  CompileCtx ctx = {.curr_pos = stream_begin,
                    .stream_begin = stream_begin,
                    .insts_out = &insts,
                    .filename = filename};

  for (;;) {
    int ret_code;
    if ((ret_code = compile_inst(&ctx)) != 0) {
      if (ret_code == RET_CODE_NORET)
        goto done;
      return ret_code;
    }
  }

done:
  *insts_out = insts;
  return RET_CODE_OK;
}

int read_file(char *file_path, char **contents_out) {
  FILE *file = fopen(file_path, "rb");

  ERR_IF(!file, "File error: Could not open file '%s'", file_path);

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    print_err("File error: Could not obtain file size for '%s'", file_path);
    return RET_CODE_ERR;
  }

  long filesize = ftell(file);

  if (filesize < 0) {
    fclose(file);
    print_err("File error: Could not determine the file size for '%s'",
              file_path);
    return RET_CODE_ERR;
  }

  rewind(file);
  char *buffer = malloc(filesize + 1);
  size_t read_size = fread(buffer, 1, filesize, file);

  if (read_size != (unsigned long)filesize) {
    free(buffer);
    fclose(file);
    print_err("File error: Could not read file '%s'", file_path);
    return RET_CODE_ERR;
  }

  buffer[filesize] = '\0';

  fclose(file);
  *contents_out = buffer;

  return RET_CODE_OK;
}

int read_file_insts(char *file_path, inst_ty **contents_out,
                    long *insts_count_out) {
  FILE *file = fopen(file_path, "rb");

  ERR_IF(!file, "File error: Could not open file '%s'", file_path);

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    print_err("File error: Could not obtain file size for '%s'", file_path);
    return RET_CODE_ERR;
  }

  long filesize = ftell(file);
  if (filesize < 0) {
    fclose(file);
    print_err("File error: Could not determine the file size for '%s'",
              file_path);
    return RET_CODE_ERR;
  }

  if (filesize < FILE_SIG_SIZE) {
    fclose(file);
    fprintf(stderr, "File error: File must have '%s' signature\n", FILE_SIG);
    return RET_CODE_ERR;
  }

  long instruction_size = filesize - FILE_SIG_SIZE;

  if (instruction_size % sizeof(inst_ty) != 0) {
    fclose(file);
    fprintf(stderr,
            "File error: The number of instruction bytes must be a multiple of "
            "%zu\n",
            sizeof(inst_ty));
    return RET_CODE_ERR;
  }

  rewind(file);

  char signature[FILE_SIG_SIZE];
  if (fread(signature, 1, FILE_SIG_SIZE, file) != FILE_SIG_SIZE) {
    fclose(file);
    print_err("File error: Could not read the file signature from '%s'",
              file_path);
    return RET_CODE_ERR;
  }

  if (memcmp(signature, FILE_SIG, FILE_SIG_SIZE) != 0) {
    fclose(file);
    fprintf(stderr, "File error: Invalid file signature for '%s'\n", file_path);
    return RET_CODE_ERR;
  }

  inst_ty *buffer = malloc(instruction_size);
  if (!buffer) {
    fclose(file);
    print_err("Memory error: Could not allocate memory for instructions");
    return RET_CODE_ERR;
  }

  if (fread(buffer, 1, instruction_size, file) != (size_t)instruction_size) {
    free(buffer);
    fclose(file);
    print_err("File error: Could not read instructions from '%s'", file_path);
    return RET_CODE_ERR;
  }

  fclose(file);
  *contents_out = buffer;
  *insts_count_out = instruction_size / sizeof(inst_ty);

  return RET_CODE_OK;
}
