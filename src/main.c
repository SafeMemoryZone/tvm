#include "assembler.h"
#include "common.h"
#include "vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char tvm_file_signature[] = "TVM";

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

  while(i < argc) {
    int has_next = argc - i > 1;
    char *arg = argv[i];

    if(strcmp(arg, "-c") == 0 || strcmp(arg, "-compile") == 0) {
      args.action = ACTION_COMPILE;
      i++;
    }

    else if(strcmp(arg, "-o") == 0 || strcmp(arg, "-out") == 0 || strcmp(arg, "-output") == 0) {
      if(!has_next) {
        fprintf(stderr, "Args error: Expected output file after '%s'\n", arg);
        return ERR_CODE_ARGS_ERR;
      }
      args.output = argv[i + 1];
      i += 2;
    }

    else if(*arg == '-') {
      fprintf(stderr, "Args error: Unknown option '%s'\n", arg);
      return ERR_CODE_ARGS_ERR;
    }
    else {
      positional_args_start = i;
      break;
    }
  }

  if(positional_args_start == argc) {
    fprintf(stderr, "Args error: Expected input file\n");
    return ERR_CODE_ARGS_ERR;
  }

  args.input = argv[positional_args_start++];

  for(; positional_args_start < argc; positional_args_start++)
    printf("Args warning: Ignoring positional argument '%s'\n", argv[positional_args_start]);

  *args_out = args;
  return ERR_CODE_OK;
}

int tvm_compile(Args args) {
  char *contents;
  int code;

  if((code = read_file(args.input, &contents)) != 0)
    return code;

  InstsOut insts;
  if((code = assembler_compile(contents, &insts)) != 0)
    return code;

  free(contents);

  FILE *file;

  if(args.output) {
    file = fopen(args.output, "w+b");
    if(!file) {
      print_err("File error: Could not open or create output file '%s'", args.output);
      return ERR_CODE_FS_ERR;
    }
  }
  else {
    file = fopen("out.tvm", "w+b");
    if(!file) {
      print_err("File error: Could not open or create output file 'out.tvm'");
      return ERR_CODE_FS_ERR;
    }

    args.output = "out.tvm";
  }

  if(fwrite(tvm_file_signature, 1, strlen(tvm_file_signature), file) < strlen(tvm_file_signature)) {
    fclose(file);
    print_err("File error: Could not write to file '%s'", args.output);
    return ERR_CODE_FS_ERR;
  }

  if(fwrite(insts.insts, 1, insts.size, file) < insts.size) {
    fclose(file);
    print_err("File error: Could not write to file '%s'", args.output);
    return ERR_CODE_FS_ERR;
  }

  fclose(file);
  free(insts.insts);

  return ERR_CODE_OK;
}

int tvm_run(Args args) {
  char *contents;
  int code;

  if((code = read_file(args.input, &contents)) != 0)
    return code;

  size_t file_size = strlen(contents);
  size_t file_signature_size = strlen(tvm_file_signature);

  if(file_size < file_signature_size) {
    fprintf(stderr, "File error: File must have '%s' signature\n", tvm_file_signature);
    return ERR_CODE_FILE_FORMAT_ERR;
  }

  if(strncmp(contents, tvm_file_signature, file_signature_size) != 0) {
    fprintf(stderr, "File error: File must have '%s' signature\n", tvm_file_signature);
    return ERR_CODE_FILE_FORMAT_ERR;
  }

  VmCtx ctx;
  int program_ret_code;
  vm_init_ctx(&ctx, (uint32_t *) (contents + file_signature_size), file_size - file_signature_size); 
  if((code = vm_run(&ctx, &program_ret_code)) != 0)
    return code;

  printf("Program returned %d\n", program_ret_code);

  free(contents);

  return ERR_CODE_OK;
}

int main(int argc, char **argv) {
  Args args;
  int code = ERR_CODE_OK;

  if((code = parse_cmd_args(argc, argv, &args)) != 0) goto exit;

  switch(args.action) {
    case ACTION_COMPILE:
      if((code = tvm_compile(args)) != 0) goto exit;
      break;
    case ACTION_RUN:
      if((code = tvm_run(args)) != 0) goto exit;
      break;
  }

exit:
  if(code != 0)
    printf("\nVM exited with error code: %s\n", err_as_str(code));
  else
    printf("\nVM exited successfully\n");

  return code;
}
