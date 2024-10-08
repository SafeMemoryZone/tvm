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
  char *input_file;
  char *output_file;
} Args;

int parse_cmd_args(int argc, char **argv, Args *args_out) {
  Args args = {.action = ACTION_RUN, .input_file = NULL, .output_file = NULL};
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
      args.output_file = argv[i + 1];
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
  args.input_file = argv[positional_args_start++];

  for (; positional_args_start < argc; positional_args_start++)
    printf("Args warning: Ignoring positional argument '%s'\n", argv[positional_args_start]);

  *args_out = args;
  return RET_CODE_OK;
}

int tvm_compile(Args args) {
  char *file_contents;
  int tmp_ret_code;

  if ((tmp_ret_code = read_file(args.input_file, &file_contents)) != 0) return tmp_ret_code;

  InstsOut insts;

  if ((tmp_ret_code = assembler_compile(args.input_file, file_contents, &insts)) != 0) {
    free(file_contents);
    return tmp_ret_code;
  }

  free(file_contents);

  FILE *file;

  if (args.output_file) {
    file = fopen(args.output_file, "w+b");
    ERR_IF(!file, "File error: Could not open or create output file '%s'", args.output_file);
  }
  else {
    file = fopen("out.tvm", "w+b");
    ERR_IF(!file, "File error: Could not open or create output file 'out.tvm'");
    args.output_file = "out.tvm";
  }

  if (fwrite(FILE_SIG, 1, strlen(FILE_SIG), file) < strlen(FILE_SIG)) {
    fclose(file);
    print_err("File error: Could not write to file '%s'", args.output_file);
    return RET_CODE_ERR;
  }

  if (fwrite(insts.insts, 1, insts.size * sizeof(inst_ty), file) < insts.size * sizeof(inst_ty)) {
    fclose(file);
    print_err("File error: Could not write to file '%s'", args.output_file);
    return RET_CODE_ERR;
  }

  fclose(file);
  free(insts.insts);

  return RET_CODE_OK;
}

int tvm_run(Args args) {
  uint32_t *bin_contents;
  int tmp_ret_code;
  long insts_size;

  if ((tmp_ret_code = read_file_insts(args.input_file, &bin_contents, &insts_size)) != 0)
    return tmp_ret_code;

  VmCtx ctx = {0};
  int program_ret_code;

  vm_init_ctx(&ctx, bin_contents, insts_size);
  if ((tmp_ret_code = vm_run(&ctx, &program_ret_code)) != 0) return tmp_ret_code;

  printf(
      "r0: %lld\nr1: %lld\nr2: %lld\nr3: %lld\nr4: %lld\nr5: %lld\nr6: %lld\nr7:"
      " %lld\n",
      ctx.regs[0], ctx.regs[1], ctx.regs[2], ctx.regs[3], ctx.regs[4], ctx.regs[5], ctx.regs[6],
      ctx.regs[7]);
  printf("Program returned %d\n", program_ret_code);

  free(bin_contents);

  return program_ret_code;
}

int main(int argc, char **argv) {
  Args args;
  int tmp_ret_code = RET_CODE_OK;

  if ((tmp_ret_code = parse_cmd_args(argc, argv, &args)) != 0) goto exit;

  switch (args.action) {
    case ACTION_COMPILE:
      tmp_ret_code = tvm_compile(args);
      break;
    case ACTION_RUN:
      tmp_ret_code = tvm_run(args);
      break;
  }

exit:
  return tmp_ret_code;
}
