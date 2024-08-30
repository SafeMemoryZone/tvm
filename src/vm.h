#ifndef VM_H
#define VM_H
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
  VmWord stack[STACK_SIZE];
  VmWord regs[8];
  size_t insts_size;
  size_t sp;
  uint32_t *insts;
  uint32_t *ip;
} VmCtx;

void vm_init_ctx(VmCtx *ctx, uint32_t *insts, size_t insts_size);
int vm_run(VmCtx *ctx, int *program_ret_code);
#endif // VM_H
