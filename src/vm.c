#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "error.h"

#define INST_MNEMONIC(inst) (inst_extract_bits(inst, FIELD_MNEMONIC, false))

const InstField FIELD_MNEMONIC = {0, 8};
const InstField FIELD_EXIT_CODE = {8, 8};

const InstField FIELD_BINOP_DST = {8, 3};
const InstField FIELD_BINOP_OP1 = {11, 3};
const InstField FIELD_BINOP_IS_IMM = {14, 1};
const InstField FIELD_BINOP_OP2 = {15, 3};
const InstField FIELD_BINOP_IMM = {15, 17};

const InstField FIELD_MOV_DST = {8, 3};
const InstField FIELD_MOV_IS_IMM = {11, 1};
const InstField FIELD_MOV_IMM = {12, 20};
const InstField FIELD_MOV_SRC = {12, 3};

const InstField FIELD_LOAD_DST = {8, 3};
// other fields for `load` are not defined

const InstField FIELD_JMP_OFF = {8, 24};

const InstField FIELD_INC_REG = {8, 3};
const InstField FIELD_DEC_REG = {8, 3};

const InstField FIELD_CMP_REG1 = {8, 3};
const InstField FIELD_CMP_REG2 = {11, 3};

const InstField FIELD_COND_JMP_OFF = {8, 24};

const InstField FIELD_NOT_DST = {8, 3};
const InstField FIELD_NOT_SRC = {11, 3};

int32_t inst_extract_bits(inst_ty inst, InstField field, bool signext) {
  int32_t mask = (1 << field.bit_count) - 1;
  int32_t extracted_bits = (inst >> field.start_bit) & mask;

  if (signext && (extracted_bits & (1 << (field.bit_count - 1))))
    extracted_bits |= ~((1 << field.bit_count) - 1);

  return extracted_bits;
}

void handle_inc(VmCtx *ctx, inst_ty inst) {
  int reg = inst_extract_bits(inst, FIELD_INC_REG, false);
  ctx->regs[reg]++;
}

void handle_dec(VmCtx *ctx, inst_ty inst) {
  int reg = inst_extract_bits(inst, FIELD_DEC_REG, false);
  ctx->regs[reg]--;
}

void handle_bin_op(VmCtx *ctx, inst_ty inst, char op) {
  int dst_reg = inst_extract_bits(inst, FIELD_BINOP_DST, false);
  int op1_reg_val = ctx->regs[inst_extract_bits(inst, FIELD_BINOP_OP1, false)];
  bool is_imm = inst_extract_bits(inst, FIELD_BINOP_IS_IMM, false);
  VmWord op2_val = is_imm ? inst_extract_bits(inst, FIELD_BINOP_IMM, true)
                          : ctx->regs[inst_extract_bits(inst, FIELD_BINOP_OP2, false)];

  switch (op) {
    default:
      assert(false);
    case '+':
      ctx->regs[dst_reg] = op1_reg_val + op2_val;
      break;
    case '-':
      ctx->regs[dst_reg] = op1_reg_val - op2_val;
      break;
    case '*':
      ctx->regs[dst_reg] = op1_reg_val * op2_val;
      break;
    case '/':
      ctx->regs[dst_reg] = op1_reg_val / op2_val;
      break;
    case '|':
      ctx->regs[dst_reg] = op1_reg_val | op2_val;
      break;
    case '&':
      ctx->regs[dst_reg] = op1_reg_val & op2_val;
      break;
    case '^':
      ctx->regs[dst_reg] = op1_reg_val ^ op2_val;
      break;
    case '>':
      ctx->regs[dst_reg] = op1_reg_val >> op2_val;
      break;
    case '<':
      ctx->regs[dst_reg] = op1_reg_val << op2_val;
      break;
  }
}

void handle_mov(VmCtx *ctx, inst_ty inst) {
  int dst_reg = inst_extract_bits(inst, FIELD_MOV_DST, false);
  int is_imm = inst_extract_bits(inst, FIELD_MOV_IS_IMM, false);
  VmWord op_val = is_imm ? inst_extract_bits(inst, FIELD_MOV_IMM, true)
                         : ctx->regs[inst_extract_bits(inst, FIELD_MOV_SRC, false)];
  ctx->regs[dst_reg] = op_val;
}

void handle_cmp(VmCtx *ctx, inst_ty inst) {
  VmWord reg1_val = ctx->regs[inst_extract_bits(inst, FIELD_CMP_REG1, false)];
  VmWord reg2_val = ctx->regs[inst_extract_bits(inst, FIELD_CMP_REG2, false)];

  ctx->f_zero = !reg1_val || !reg2_val;
  ctx->f_eq = reg1_val == reg2_val;
  ctx->f_greater = reg1_val > reg2_val;
  ctx->f_smaller = reg1_val < reg2_val;
}

int handle_load(VmCtx *ctx, inst_ty inst) {
  int dst_reg = inst_extract_bits(inst, FIELD_LOAD_DST, false);
  inst_ty *first_invalid_inst = ctx->ip + ctx->insts_count;

  if (ctx->ip - 2 >= first_invalid_inst) {
    print_err("VM error: Instruction pointer went past last instruction");
    return RET_CODE_ERR;
  }

  VmWord num_to_load = ((uint64_t)ctx->ip[2] << 32) | (uint64_t)ctx->ip[1];
  ctx->regs[dst_reg] = num_to_load;

  ctx->ip += 2;
  return RET_CODE_OK;
}

void handle_not(VmCtx *ctx, inst_ty inst) {
  int dst_reg = inst_extract_bits(inst, FIELD_NOT_DST, false);
  VmWord src_reg_val = inst_extract_bits(inst, FIELD_NOT_SRC, true);

  ctx->regs[dst_reg] = ~src_reg_val;
}

int handle_jmp(VmCtx *ctx, inst_ty inst) {
  int32_t jmp_off = inst_extract_bits(inst, FIELD_JMP_OFF, true);
  inst_ty *first_invalid_inst = ctx->ip + ctx->insts_count;

  if (ctx->ip + jmp_off < ctx->first_inst || ctx->ip + jmp_off > first_invalid_inst) {
    print_err("VM error: Jump instruction points to an invalid location");
    return RET_CODE_ERR;
  }

  ctx->ip += jmp_off - 1;  // it will be incremented later
  return RET_CODE_OK;
}

int handle_cond_jmp(VmCtx *ctx, inst_ty inst) {
  int32_t jmp_off = inst_extract_bits(inst, FIELD_COND_JMP_OFF, true);
  inst_ty *first_invalid_inst = ctx->ip + ctx->insts_count;

  if (ctx->ip + jmp_off < ctx->first_inst || ctx->ip + jmp_off > first_invalid_inst) {
    print_err("VM error: Jump instruction points to an invalid location");
    return RET_CODE_ERR;
  }

  switch (INST_MNEMONIC(inst)) {
    default:
      assert(false);
    case MNEMONIC_JMP_GREATER:
      if (ctx->f_greater) ctx->ip += jmp_off - 1;
      break;
    case MNEMONIC_JMP_LOWER:
      if (ctx->f_smaller) ctx->ip += jmp_off - 1;
      break;
    case MNEMONIC_JMP_EQ:
      if (ctx->f_eq) ctx->ip += jmp_off - 1;
      break;
    case MNEMONIC_JMPZ:
      if (ctx->f_zero) ctx->ip += jmp_off - 1;
      break;
  }

  return RET_CODE_OK;
}

void handle_exit(inst_ty inst, int *program_ret_code_out) {
  *program_ret_code_out = inst_extract_bits(inst, FIELD_EXIT_CODE, false);
}

int execute_instruction(VmCtx *ctx, int *program_ret_code_out) {
  inst_ty inst = *ctx->ip;

  switch (INST_MNEMONIC(inst)) {
    case MNEMONIC_EXIT:
      handle_exit(inst, program_ret_code_out);
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
    case MNEMONIC_OR:
      handle_bin_op(ctx, inst, '|');
      break;
    case MNEMONIC_AND:
      handle_bin_op(ctx, inst, '&');
      break;
    case MNEMONIC_XOR:
      handle_bin_op(ctx, inst, '^');
      break;
    case MNEMONIC_SHR:
      handle_bin_op(ctx, inst, '>');
      break;
    case MNEMONIC_SHL:
      handle_bin_op(ctx, inst, '<');
      break;
    case MNEMONIC_NOT:
      handle_not(ctx, inst);
      break;
    case MNEMONIC_MOV:
      handle_mov(ctx, inst);
      break;
    case MNEMONIC_LOAD: {
      int tmp_ret_code;
      if ((tmp_ret_code = handle_load(ctx, inst)) != 0) return tmp_ret_code;
      break;
    }
    case MNEMONIC_JMP: {
      int tmp_ret_code;
      if ((tmp_ret_code = handle_jmp(ctx, inst)) != 0) return tmp_ret_code;
      break;
    }
    case MNEMONIC_INC:
      handle_inc(ctx, inst);
      break;
    case MNEMONIC_DEC:
      handle_dec(ctx, inst);
      break;
    case MNEMONIC_CMP:
      handle_cmp(ctx, inst);
      break;
    case MNEMONIC_JMP_GREATER:
    case MNEMONIC_JMP_LOWER:
    case MNEMONIC_JMP_EQ:
    case MNEMONIC_JMPZ: {
      int tmp_ret_code;
      if ((tmp_ret_code = handle_cond_jmp(ctx, inst)) != 0) return tmp_ret_code;
      break;
    }
    default:
      print_err("VM error: Unknown mnemonic with opcode %d", INST_MNEMONIC(inst));
      return RET_CODE_ERR;
  }

  return RET_CODE_OK;
}

void vm_init_ctx(VmCtx *ctx, inst_ty *insts, size_t insts_count) {
  *ctx = (VmCtx){0};
  ctx->first_inst = insts;
  ctx->insts_count = insts_count;
  ctx->ip = insts;
}

int vm_run(VmCtx *ctx, int *program_ret_code_out) {
  inst_ty *first_invalid_inst = ctx->ip + ctx->insts_count;

  while (ctx->ip < first_invalid_inst) {
    int tmp_ret_code = execute_instruction(ctx, program_ret_code_out);
    if (tmp_ret_code == RET_CODE_ERR) return RET_CODE_ERR;
    if (tmp_ret_code == RET_CODE_NORET) break;
    ctx->ip++;
  }

  ERR_IF(ctx->ip >= first_invalid_inst,
         "VM error: Instruction pointer went past last instruction (probably forgot to exit)");
  return RET_CODE_OK;
}
