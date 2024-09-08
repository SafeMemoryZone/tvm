#ifndef VM_H
#define VM_H
#include <stddef.h>
#include <stdint.h>

#define FILE_SIG_SIZE 3
#define FILE_SIG "TVM"

#define STACK_SIZE 2048
#define REGS_COUNT 8

typedef uint32_t inst_ty;

enum Mnemonic {
  MNEMONIC_EXIT = 0,
  MNEMONIC_ADD,
  MNEMONIC_SUB,
  MNEMONIC_MUL,
  MNEMONIC_DIV,
};

typedef union {
  int64_t i64;
  double f64;
  void *ptr;
} VmWord;

typedef struct {
  inst_ty *insts;
  size_t insts_count;
  inst_ty *ip;
  VmWord *sp;

  VmWord stack[STACK_SIZE];
  VmWord regs[REGS_COUNT];
} VmCtx;

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count);
int vm_run(VmCtx *ctx, int *program_ret_code);
#endif // VM_H
