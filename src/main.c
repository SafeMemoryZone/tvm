#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

char *stringify_exit_code(int code) {
  switch(code) {
    default: return "Ok";
    case EXIT_INVALID_IP:
      return "Invalid ip";
    case EXIT_INVALID_INSTRUCTION:
      return "Invalid instruction";
    case EXIT_MISSING_FLAG:
      return "Missing instruction flag";
  }
}

int init_vm_and_run(uint32_t *instructions, size_t instructions_size) {
  VmCtx ctx;
  init_vm_ctx(&ctx, instructions, instructions_size);
  int exit_code = run_vm(&ctx);
  printf("VM exited with status: %s\n", stringify_exit_code(exit_code));
  
  return exit_code;
}

int main(int argc, char **argv) {
  // Testing code
  uint32_t *instructions = malloc(sizeof(uint32_t) * 2048);
  instructions[0] = CONSTURCT_INSTRUCTION(MNEMONIC_LOAD_CONST, R0, 0, 0, FLAG_B32);
  instructions[1] = 100;
  instructions[2] = CONSTURCT_INSTRUCTION(MNEMONIC_LOAD_CONST, R1, 0, 0, FLAG_B32 | FLAG_SIGN_EXT);
  instructions[3] = -200;
  instructions[4] = CONSTURCT_INSTRUCTION(MNEMONIC_ADD, R0, R1, R2, 0);
  instructions[5] = CONSTURCT_INSTRUCTION(MNEMONIC_HALT, 0, 0, 0, 0);

  return init_vm_and_run(instructions, 2048);
}
