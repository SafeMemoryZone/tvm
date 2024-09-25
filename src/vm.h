#ifndef VM_H
#define VM_H
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

typedef union {
  int64_t i64;
  double f64;
  void *ptr;
} VmWord;

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
};

typedef struct {
  inst_ty *first_inst;
  size_t insts_count;
  inst_ty *ip;
  VmWord *sp;

  VmWord stack[STACK_SIZE];
  VmWord regs[REGS_COUNT];
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

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count);
int vm_run(VmCtx *ctx, int *program_ret_code_out);
#endif  // VM_H
