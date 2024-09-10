#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "error.h"
#include "vm.h"

enum Action {
  ACTION_COMPILE,
  ACTION_RUN,
};

typedef struct {
  int action;
  char *input;
  char *output;
} Args;

int parse_cmd_args(int argc, char **argv, Args *args_out) {
  Args args = {.action = ACTION_RUN, .input = NULL, .output = NULL};
  int i = 1;
  int positional_args_start = argc;

  while (i < argc) {
    int has_next = argc - i > 1;
    char *arg = argv[i];

    if (strcmp(arg, "-c") == 0 || strcmp(arg, "-compile") == 0) {
      args.action = ACTION_COMPILE;
      i++;
    }

    else if (strcmp(arg, "-o") == 0 || strcmp(arg, "-out") == 0 || strcmp(arg, "-output") == 0) {
      ERR_IF(!has_next, "Args error: Expected output file after '%s'", arg);
      args.output = argv[i + 1];
      i += 2;
    }

    else if (*arg == '-') {
      fprintf(stderr, "Args error: Unknown option '%s'\n", arg);
      return RET_CODE_ERR;
    }
    else {
      positional_args_start = i;
      break;
    }
  }

  ERR_IF(positional_args_start == argc, "Args error: Expected input file");
  args.input = argv[positional_args_start++];

  for (; positional_args_start < argc; positional_args_start++)
    printf("Args warning: Ignoring positional argument '%s'\n", argv[positional_args_start]);

  *args_out = args;
  return RET_CODE_OK;
}

int tvm_compile(Args args) {
  char *contents;
  int ret_code;

  if ((ret_code = read_file(args.input, &contents)) != 0) return ret_code;

  InstsOut insts;

  if ((ret_code = assembler_compile(args.input, contents, &insts)) != 0) {
    free(contents);
    return ret_code;
  }

  free(contents);

  FILE *file;

  if (args.output) {
    file = fopen(args.output, "w+b");
    ERR_IF(!file, "File error: Could not open or create output file '%s'", args.output);
  }
  else {
    file = fopen("out.tvm", "w+b");
    ERR_IF(!file, "File error: Could not open or create output file 'out.tvm'");
    args.output = "out.tvm";
  }

  if (fwrite(FILE_SIG, 1, strlen(FILE_SIG), file) < strlen(FILE_SIG)) {
    fclose(file);
    print_err("File error: Could not write to file '%s'", args.output);
    return RET_CODE_ERR;
  }

  if (fwrite(insts.insts, 1, insts.size, file) < insts.size) {
    fclose(file);
    print_err("File error: Could not write to file '%s'", args.output);
    return RET_CODE_ERR;
  }

  fclose(file);
  free(insts.insts);

  return RET_CODE_OK;
}

int tvm_run(Args args) {
  uint32_t *contents;
  int ret_code;
  long insts_size;

  if ((ret_code = read_file_insts(args.input, &contents, &insts_size)) != 0) return ret_code;

  VmCtx ctx = {0};
  int program_ret_ret_code;
  vm_init_ctx(&ctx, contents, insts_size);
  if ((ret_code = vm_run(&ctx, &program_ret_ret_code)) != 0) return ret_code;

  printf("r0 : %lld, r1: %lld, r2: %lld\n", ctx.regs[0].i64, ctx.regs[1].i64, ctx.regs[2].i64);
  printf("Program returned %d\n", program_ret_ret_code);

  free(contents);

  return program_ret_ret_code;
}

int main(int argc, char **argv) {
  Args args;
  int ret_code = RET_CODE_OK;

  if ((ret_code = parse_cmd_args(argc, argv, &args)) != 0) goto exit;

  switch (args.action) {
    case ACTION_COMPILE:
      ret_code = tvm_compile(args);
      break;
    case ACTION_RUN:
      ret_code = tvm_run(args);
      break;
  }

exit:
  return ret_code;
}
