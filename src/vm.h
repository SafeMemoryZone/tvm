#ifndef VM_H
#define VM_H
#include <stddef.h>
#include <stdint.h>

#define FLAG_UNSIGNED 1
#define FLAG_FLOAT (1 << 1)
#define FLAG_B32 (1 << 2)
#define FLAG_B64 (1 << 3)
#define FLAG_SIGN_EXT (1 << 4)

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

#define CONSTURCT_INSTRUCTION(_mnemonic, _op1, _op2, _op3, _flags) (((_flags) << 17) | ((_op3) << 14) | ((_op2) << 11) | ((_op1) << 8) | (_mnemonic))

#define STACK_SIZE 2048

/*
 * Bit Structure of an instruction:
 *  Bits 0  - 7   : Mnemonic
 *  Bits 8  - 10  : Operator 1
 *  Bits 11 - 13  : Operator 2
 *  Bits 14 - 16  : Operator 3
 *  Bits 17 - 31  : Flags
 */

enum Mnemonic {
  MNEMONIC_HALT = 0,
  MNEMONIC_ADD,
  MNEMONIC_SUB,
  MNEMONIC_MUL,
  MNEMONIC_DIV,
  MNEMONIC_LOAD_CONST,
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
} ComputationalElement;

typedef struct {
  ComputationalElement stack[STACK_SIZE];
  ComputationalElement regs[8];

  size_t instructions_size;
  uint32_t *instructions;
  uint32_t *ip;
  int sp;
} VmCtx;

void init_vm_ctx(VmCtx *ctx, uint32_t *instructions, size_t instructions_size);
int run_vm(VmCtx *ctx);
#endif // VM_H
