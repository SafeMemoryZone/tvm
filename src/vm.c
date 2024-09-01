#include "vm.h"
#include "common.h"
#include <stdio.h>
#include <stdint.h>

#define INST_MNEMONIC(inst) ((inst) & 0xFF)
#define INST_FIRST_BYTE(inst) (((inst) & 0xFF00) >> 8)

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count) {
  ctx->insts = insts;
  ctx->insts_count = insts_count;
  ctx->ip = insts;
}

int vm_run(VmCtx *ctx, int *program_ret_ret_code) {
  uint32_t *invalid_inst_ptr = ctx->ip + ctx->insts_count;

  for(;;) {
    if(ctx->ip >= invalid_inst_ptr) {
      fprintf(stderr, "VM error: Instruction ptr went past last instruction\n");
      return RET_CODE_ERR;
    }
    inst_ty inst = *ctx->ip;
    switch(INST_MNEMONIC(inst)) {
      default:
        fprintf(stderr, "VM error: Unknown mnemonic with opret_code %d\n", INST_MNEMONIC(inst));
        return RET_CODE_ERR;
      case MNEMONIC_EXIT:
        *program_ret_ret_code = INST_FIRST_BYTE(inst);
        goto vm_end;
    }
  }

vm_end:
  return RET_CODE_OK;
}
