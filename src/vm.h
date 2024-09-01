#ifndef VM_H
#define VM_H
#include "common.h"
#include <stddef.h>
#include <stdint.h>

#define STACK_SIZE 2048

enum Mnemonic {
  MNEMONIC_EXIT = 0,
};

typedef union {
  int64_t i64;
  uint64_t u64;
  double f64;
  void *ptr;
} VmWord;

typedef struct {
  inst_ty *insts;
  size_t insts_count;
  inst_ty *ip;
  VmWord *sp;

  VmWord stack[STACK_SIZE];
  VmWord regs[8];
} VmCtx;

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count);
int vm_run(VmCtx *ctx, int *program_ret_code);
#endif // VM_H
