#ifndef VM_H
#define VM_H
#include <stddef.h>
#include <stdint.h>

#define FLAG_UNSIGNED 1
#define FLAG_FLOAT (1 << 1)
#define FLAG_B32 (1 << 2)
#define FLAG_B64 (1 << 3)
#define FLAG_SIGN_EXT (1 << 4)

#define STACK_SIZE 2048

/*
 *  Format for instruction(s) [halt]:
 *  Bits 0  - 7   : Mnemonic
 */

/*
 *  Format for instruction(s) [add, sub, mul, div]:
 *  Bits 0  - 7   : Mnemonic
 *  Bits 8  - 10  : Register 1
 *  Bits 11 - 13  : Register 2
 *  Bits 14 - 16  : Register 3
 *  Bits 17 - 31  : Flags
 */

/*
 *  Format for instruction(s) [load_const]:
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
  EXIT_INVALID_INSTRUCTION,
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

  size_t instructions_size;
  uint32_t *instructions;
  uint32_t *ip;
  int sp;
} VmCtx;

void vm_init_ctx(VmCtx *ctx, uint32_t *instructions, size_t instructions_size);
int vm_run(VmCtx *ctx);
char *vm_stringify_exit_code(int code);
#endif // VM_H
