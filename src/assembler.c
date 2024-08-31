#include "assembler.h"
#include "common.h"
#include "vm.h"
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

typedef struct {
  char *curr_pos;
  char *stream_begin;
  InstsOut *insts_out;
} CompileCtx;

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

void print_syntax_err(CompileCtx *ctx, char *err_loc_first_char, char *err_loc_last_char, char *fmt, ...) {
  fprintf(stderr, "Assembler error: ");
  va_list args_ptr;
  va_start(args_ptr, fmt);
  vfprintf(stderr, fmt, args_ptr);
  va_end(args_ptr);

  int ln, col;
  get_curr_pos_loc(err_loc_last_char, ctx->stream_begin, &ln, &col);
  fprintf(stderr, " (line %d:%d)\n", ln, col);

  char *ln_start_ptr = err_loc_first_char;
  while (ln_start_ptr > ctx->stream_begin && *(ln_start_ptr - 1) != '\n')
    ln_start_ptr--;

  char *ln_end_ptr = ln_start_ptr;
  while (*ln_end_ptr != '\n' && *ln_end_ptr != '\0')
    ln_end_ptr++;

  fprintf(stderr, "%.*s\n", (int)(ln_end_ptr - ln_start_ptr), ln_start_ptr);

  for (char *p = ln_start_ptr; p < err_loc_first_char; p++) {
    fputc((*p == '\t') ? '\t' : ' ', stderr);
  }

  fputc('^', stderr);
  if (err_loc_last_char != NULL) {
    for (char *p = err_loc_first_char + 1; p <= err_loc_last_char; p++)
      fputc('~', stderr);
  }
  fprintf(stderr, "\n");
}

bool is_ident(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int get_next_tok(CompileCtx *ctx, Token *tok_out) {
  while(isspace(*ctx->curr_pos))
    ctx->curr_pos++;

  if(*ctx->curr_pos == 0)
    return RETURN_CODE_NORETURN;

  char *ident_first_char = ctx->curr_pos;
  char *ident_last_char = NULL;

  while(is_ident(*ctx->curr_pos)) {
    ident_last_char = ctx->curr_pos;
    ctx->curr_pos++;
  }

  if(ident_last_char) {
    *tok_out = (Token) {
      .first_char = ident_first_char,
      .last_char = ident_last_char,
      .ty = TT_IDENT
    };
    ctx->curr_pos = ident_last_char + 1;
    return RETURN_CODE_OK;
  }

  if(isdigit(*ctx->curr_pos)) {
    char *num_end = NULL;
    int64_t parsed_num = strtoll(ctx->curr_pos, &num_end, 0); 

    if(parsed_num == LLONG_MAX) {
      while(isdigit(*ctx->curr_pos)) {
        num_end = ctx->curr_pos;
        ctx->curr_pos++;
      }
      print_syntax_err(ctx, ctx->curr_pos, num_end, "Number overflow");
      return RETURN_CODE_ERR;
    }
    else if(parsed_num == LLONG_MIN) {
      while(isdigit(*ctx->curr_pos)) {
        num_end = ctx->curr_pos;
        ctx->curr_pos++;
      }
      print_syntax_err(ctx, ctx->curr_pos, num_end, "Number underflow");
      return RETURN_CODE_ERR;     
    }

    *tok_out = (Token) {
      .first_char = ctx->curr_pos,
        .last_char = num_end - 1,
        .ty = TT_NUM,
        .i64 = parsed_num
    };

    ctx->curr_pos = num_end;
    return RETURN_CODE_OK;
  }

  return RETURN_CODE_ERR;
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

bool cmp_mnemonic(char *mnemonic, char *first_tok_char) {
  if(strlen(first_tok_char) < strlen(mnemonic))
    return false;

  for(unsigned int i = 0; i < strlen(mnemonic); i++) {
    if(mnemonic[i] != first_tok_char[i])
      return false;
  }

  return true;
}

int compile_inst(CompileCtx *ctx) {
  Token inst;
  int code;

  if((code = get_next_tok(ctx, &inst)) != 0)
    return code;

  if(inst.ty != TT_IDENT) goto unknown_inst;

  if(cmp_mnemonic("exit", inst.first_char)) {
    Token exit_code;

    if((code = get_next_tok(ctx, &exit_code)) != 0) {
      if(code == RETURN_CODE_NORETURN) {
        print_syntax_err(ctx, inst.last_char, inst.last_char, "Expected an exit code after 'exit'");
        return RETURN_CODE_ERR;
      }
      return code;
    }
    if(exit_code.ty != TT_NUM) {
      print_syntax_err(ctx, exit_code.first_char, exit_code.last_char, "Expected exit code to be a number");
      return RETURN_CODE_ERR;
    }

    insts_out_append(ctx->insts_out, exit_code.i64 << 8 | MNEMONIC_EXIT);
    return RETURN_CODE_OK;
  }

unknown_inst:
  print_syntax_err(ctx, inst.first_char, inst.last_char, "Unknown instruction");
  return RETURN_CODE_ERR;
}

int assembler_compile(char *stream_begin, InstsOut *insts_out) {
  InstsOut insts = {0};
  CompileCtx ctx = { .curr_pos = stream_begin, .stream_begin = stream_begin, .insts_out = &insts };

  for(;;) {
    int code;
    if((code = compile_inst(&ctx)) != 0) {
      if(code == RETURN_CODE_NORETURN) goto done;
      return code;
    }
  }

done:
  *insts_out = insts;
  return RETURN_CODE_OK;
}

int read_file(char *file_path, char **contents_out) {
  FILE *file = fopen(file_path, "rb");

  if (!file) {
    print_err("File error: Could not open file '%s'", file_path);
    return RETURN_CODE_ERR;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    print_err("File error: Could not obtain file size for '%s'", file_path);
    return RETURN_CODE_ERR;
  }

  long filesize = ftell(file);
  if (filesize < 0) {
    fclose(file);
    print_err("File error: Could not determine the file size for '%s'", file_path);
    return RETURN_CODE_ERR;
  }

  rewind(file);

  char *buffer = malloc(filesize + 1);
  size_t read_size = fread(buffer, 1, filesize, file);

  if (read_size != (unsigned long) filesize) {
    free(buffer);
    fclose(file);
    print_err("File error: Could not read file '%s'", file_path);
    return RETURN_CODE_ERR;
  }

  buffer[filesize] = '\0';
  fclose(file);
  *contents_out = buffer;

  return RETURN_CODE_OK;
}
