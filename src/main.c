#include "assembler.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Action {
  ACTION_COMPILE,
  ACTION_RUN,
};

typedef struct {
  int action;
  char *input;
  char *output;
} Args;

Args parse_cmd_args(int argc, char **argv) {
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
      expect(has_next, "Args error: Expected output file after '%s'\n", arg);
      args.output = argv[i + 1];
      i += 2;
    }

    else if(*arg == '-')
      fatal_error("Args error: Unknown option '%s'\n", arg);

    else {
      positional_args_start = i;
      break;
    }
  }

  if(positional_args_start == argc)
    fatal_error("Args error: Expected input file\n");

  args.input = argv[positional_args_start++];

  for(; positional_args_start < argc; positional_args_start++)
    printf("Args warning: Ignoring positional argument '%s'\n", argv[positional_args_start]);

  return args;
}

void tvm_compile(Args args) {
  char *stream = read_file(args.input);
  assembler_compile(stream, NULL, NULL);
  free(stream);
}

void tvm_run(Args args) {
  // TODO: implement
}

int main(int argc, char **argv) {
  Args args = parse_cmd_args(argc, argv);

  switch(args.action) {
    default: fatal_error("Args error: No action was specified\n");
    case ACTION_RUN:
      tvm_run(args);
      break;
    case ACTION_COMPILE:
      tvm_compile(args);
      break;
  }
}
