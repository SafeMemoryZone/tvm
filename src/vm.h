#ifndef VM_H
#define VM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FILE_SIG_SIZE 3
#define FILE_SIG "TVM"

#define STACK_SIZE 2048
#define REGS_COUNT 8

typedef uint32_t inst_ty;

typedef struct {
  int start_bit;
  int bit_count;
} InstField;

typedef int64_t VmWord;

// all instructions have 4 bytes except a few outliers (load)
enum Mnemonic {
  MNEMONIC_EXIT = 0,
  MNEMONIC_ADD,
  MNEMONIC_SUB,
  MNEMONIC_MUL,
  MNEMONIC_DIV,
  MNEMONIC_MOV,
  MNEMONIC_LOAD,
  MNEMONIC_JMP,
  MNEMONIC_INC,
  MNEMONIC_DEC,
  MNEMONIC_CMP,
  MNEMONIC_JMP_GREATER,
  MNEMONIC_JMP_LOWER,
  MNEMONIC_JMP_EQ,
  MNEMONIC_JMPZ,
  MNEMONIC_OR,
  MNEMONIC_AND,
  MNEMONIC_XOR,
  MNEMONIC_SHR,
  MNEMONIC_SHL,
  MNEMONIC_NOT,
};

typedef struct {
  VmWord stack[STACK_SIZE];
  VmWord regs[REGS_COUNT];
  inst_ty *first_inst;
  size_t insts_count;
  inst_ty *ip;
  VmWord *sp;

  bool f_zero : 1;
  bool f_greater : 1;
  bool f_smaller : 1;
  bool f_eq : 1;
} VmCtx;

extern const InstField FIELD_MNEMONIC;
extern const InstField FIELD_EXIT_CODE;

extern const InstField FIELD_BINOP_DST;
extern const InstField FIELD_BINOP_OP1;
extern const InstField FIELD_BINOP_IS_IMM;
extern const InstField FIELD_BINOP_OP2;
extern const InstField FIELD_BINOP_IMM;

extern const InstField FIELD_MOV_DST;
extern const InstField FIELD_MOV_IS_IMM;
extern const InstField FIELD_MOV_IMM;
extern const InstField FIELD_MOV_SRC;
extern const InstField FIELD_LOAD_DST;
extern const InstField FIELD_JMP_OFF;

extern const InstField FIELD_INC_REG;
extern const InstField FIELD_DEC_REG;

extern const InstField FIELD_CMP_REG1;
extern const InstField FIELD_CMP_REG2;

extern const InstField FIELD_COND_JMP_OFF;

extern const InstField FIELD_NOT_DST;
extern const InstField FIELD_NOT_SRC;

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count);
int vm_run(VmCtx *ctx, int *program_ret_code_out);
#endif  // VM_H
