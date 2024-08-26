#ifndef VM_H
#define VM_H
#include <stddef.h>
#include <stdint.h>

#define FLAG_UNSIGNED 1
#define FLAG_FLOAT (1 << 1)
#define FLAG_32 (1 << 2)
#define FLAG_64 (1 << 3)

#define STACK_SIZE 2048

/*
 *  Format for inst(s) [halt]:
 *  Bits 0  - 7   : Mnemonic
 */

/*
 *  Format for inst(s) [add, sub, mul, div]:
 *  Bits 0  - 7   : Mnemonic
 *  Bits 8  - 10  : Register 1
 *  Bits 11 - 13  : Register 2
 *  Bits 14 - 16  : Register 3
 *  Bits 17 - 31  : Flags
 */

/*
 *  Format for inst(s) [load_const]:
 *  Bits 0  - 7   : Mnemonic
 *  Bits 8  - 10  : Register 1
 *  Bits 17 - 31  : Flags
 *  (Next 4 or 8 bytes specify the value)
 */

enum Mnemonic {
  MNEMONIC_HALT = 0,
  MNEMONIC_ADD,
  MNEMONIC_SUB,
  MNEMONIC_MUL,
  MNEMONIC_DIV,
  MNEMONIC_LOAD_CONST,
};

enum Register {
  REG_0,
  REG_1,
  REG_2,
  REG_3,
  REG_4,
  REG_5,
  REG_6,
  REG_7,
};

enum ExitCode {
  EXIT_OK = 0,
  EXIT_INVALID_IP,
  EXIT_INVALID_inst,
  EXIT_MISSING_FLAG,
};

typedef union {
  int64_t int_num;
  double float_num;
  void* ptr;
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
int vm_run(VmCtx *ctx);
char *vm_stringify_exit_code(int code);
#endif // VM_H
