#include "vm.h"
#include "error.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define INST_MNEMONIC(inst) (extract_bits(inst, FIELD_MNEMONIC, false))

typedef struct {
  int start_bit;
  int bit_count;
} InstField;

static const InstField FIELD_MNEMONIC = { 0, 8 };
static const InstField FIELD_EXIT_CODE = { 8, 8 };

static const InstField FIELD_BINOP_DST = { 8, 3 };
static const InstField FIELD_BINOP_OP1 = { 11, 3 };
static const InstField FIELD_BINOP_IS_IMM = { 14, 1 };
static const InstField FIELD_BINOP_OP2 = { 15, 3 };
static const InstField FIELD_BINOP_IMM = { 15, 18 };

int32_t extract_bits(uint32_t value, InstField field, bool signext) {
  int32_t mask = (1 << field.bit_count) - 1;
  int32_t extracted = (value >> field.start_bit) & mask;

  if (signext && (extracted & (1 << (field.bit_count - 1))))
    extracted |= ~((1 << field.bit_count) - 1);

  return extracted;
}

static void handle_bin_op(VmCtx *ctx, uint32_t inst, char op) {
  int dst = extract_bits(inst, FIELD_BINOP_DST, false);
  int op1 = ctx->regs[extract_bits(inst, FIELD_BINOP_OP1, false)].i64;
  bool is_imm = extract_bits(inst, FIELD_BINOP_IS_IMM, false);
  int op2 = is_imm ? extract_bits(inst, FIELD_BINOP_IMM, true)
    : ctx->regs[extract_bits(inst, FIELD_BINOP_OP2, false)].i64;

  switch (op) {
    default: assert(false);
    case '+':
      ctx->regs[dst].i64 = op1 + op2;
      break;
    case '-':
      ctx->regs[dst].i64 = op1 - op2;
      break;
    case '*':
      ctx->regs[dst].i64 = op1 * op2;
      break;
    case '/':
      ctx->regs[dst].i64 = op1 / op2;
      break;
  }
}

static void handle_exit(uint32_t inst, int *program_ret_ret_code) {
  *program_ret_ret_code = extract_bits(inst, FIELD_EXIT_CODE, false);
}

static int execute_instruction(VmCtx *ctx, int *program_ret_ret_code) {
  uint32_t inst = *ctx->ip;

  switch (INST_MNEMONIC(inst)) {
    case MNEMONIC_EXIT:
      handle_exit(inst, program_ret_ret_code);
      return RET_CODE_NORET;
    case MNEMONIC_ADD:
      handle_bin_op(ctx, inst, '+');
      break;
    case MNEMONIC_SUB:
      handle_bin_op(ctx, inst, '-');
      break;
    case MNEMONIC_MUL:
      handle_bin_op(ctx, inst, '*');
      break;
    case MNEMONIC_DIV:
      handle_bin_op(ctx, inst, '/');
      break;
    default:
      print_err("VM error: Unknown mnemonic with opcode %d", INST_MNEMONIC(inst));
      return RET_CODE_ERR;
  }

  return RET_CODE_OK;
}

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count) {
  ctx->insts = insts;
  ctx->insts_count = insts_count;
  ctx->ip = insts;
}

int vm_run(VmCtx *ctx, int *program_ret_ret_code) {
  uint32_t *invalid_inst_ptr = ctx->ip + ctx->insts_count;

  while (ctx->ip < invalid_inst_ptr) {
    int code = execute_instruction(ctx, program_ret_ret_code);
    if (code == RET_CODE_ERR)
      return RET_CODE_ERR;
    if(code == RET_CODE_NORET)
      break;
    ctx->ip++;
  }

  ERR_IF(ctx->ip >= invalid_inst_ptr, "VM error: Instruction pointer went past last instruction");
  return RET_CODE_OK;
}
