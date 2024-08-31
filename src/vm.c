#include "vm.h"
#include "common.h"
#include <stdio.h>
#include <stdint.h>

#define INST_MNEMONIC(inst) ((inst) & 0xFF)
#define INST_FIRST_BYTE(inst) (((inst) & 0xFF00) >> 8)

void vm_init_ctx(VmCtx *ctx, uint32_t *insts, size_t insts_size) {
  ctx->insts = insts;
  ctx->insts_size = insts_size;
  ctx->ip = insts;
}

int vm_run(VmCtx *ctx, int *program_ret_code) {
  char *invalid_byte_ptr = (char *) ctx->ip + ctx->insts_size;

  for(;;) {
    if(invalid_byte_ptr <= (char *)ctx->ip + sizeof(uint32_t)) {
      fprintf(stderr, "VM error: Instruction pointer went past last byte\n");
      return RETURN_CODE_ERR;
    } 

    uint32_t inst = *ctx->ip;
    switch(INST_MNEMONIC(inst)) {
      default:
        fprintf(stderr, "VM error: Unknown mnemonic with opcode %d\n", INST_MNEMONIC(inst));
        return RETURN_CODE_ERR;
      case MNEMONIC_EXIT:
        *program_ret_code = INST_FIRST_BYTE(inst);
        goto vm_end;
    }
  }

vm_end:
  return RETURN_CODE_OK;
}
