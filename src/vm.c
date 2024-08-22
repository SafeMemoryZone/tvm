#include "vm.h"
#include <stdint.h>
#include <string.h>

#define INSTRUCTION_MNEMONIC(instruction) ((instruction) & 0xFF)
#define INSTRUCTION_OP1(instruction) (((instruction) >> 8) & 0b111)
#define INSTRUCTION_OP2(instruction) (((instruction) >> 11) & 0b111)
#define INSTRUCTION_OP3(instruction) (((instruction) >> 14) & 0b111)

#define INSTRUCTION_FLAG_UNSIGNED(instruction) (((instruction) >> 17) & 1)
#define INSTRUCTION_FLAG_FLOAT(instruction) (((instruction) >> 18) & 1)
#define INSTRUCTION_FLAG_B32(instruction) (((instruction) >> 19) & 1)
#define INSTRUCTION_FLAG_B64(instruction) (((instruction) >> 20) & 1)
#define INSTRUCTION_FLAG_SIGN_EXT(instruction) (((instruction) >> 21) & 1)

#define BIN_OP(_ctx, _binop, _rop1, _rop2, _dst, _is_unsigned, _is_float)      \
  do {                                                                         \
    if (_is_float) {                                                           \
      _ctx->regs[_dst].float_num =                                             \
          _ctx->regs[_rop1].float_num _binop _ctx->regs[_rop2].float_num;      \
    } else {                                                                   \
      if (_is_unsigned)                                                        \
        _ctx->regs[_dst].int_num =                                             \
            (uint64_t)(_ctx->regs[_rop1].int_num) _binop (uint64_t)(           \
                _ctx->regs[_rop2].int_num);                                    \
      else                                                                     \
        _ctx->regs[_dst].int_num =                                             \
            _ctx->regs[_rop1].int_num _binop _ctx->regs[_rop2].int_num;        \
    }                                                                          \
  } while (0)

void init_vm_ctx(VmCtx *ctx, uint32_t *instructions, size_t instructions_size) {
  ctx->instructions = instructions;
  ctx->instructions_size = instructions_size;
  ctx->ip = instructions;
}

int run_vm(VmCtx *ctx) {
  for (;;) {
    if (ctx->ip >= ctx->instructions + ctx->instructions_size)
      return EXIT_INVALID_IP;

    uint32_t instruction = *ctx->ip;

    switch (INSTRUCTION_MNEMONIC(instruction)) {
    default:
      return EXIT_INVALID_INSTRUCTION;
    case MNEMONIC_HALT:
      goto exit_loop;
    case MNEMONIC_ADD:
      BIN_OP(ctx, +, INSTRUCTION_OP1(instruction), INSTRUCTION_OP2(instruction),
             INSTRUCTION_OP3(instruction),
             INSTRUCTION_FLAG_UNSIGNED(instruction),
             INSTRUCTION_FLAG_FLOAT(instruction));
      break;
    case MNEMONIC_SUB:
      BIN_OP(ctx, -, INSTRUCTION_OP1(instruction), INSTRUCTION_OP2(instruction),
             INSTRUCTION_OP3(instruction),
             INSTRUCTION_FLAG_UNSIGNED(instruction),
             INSTRUCTION_FLAG_FLOAT(instruction));
      break;
    case MNEMONIC_MUL:
      BIN_OP(ctx, *, INSTRUCTION_OP1(instruction), INSTRUCTION_OP2(instruction),
             INSTRUCTION_OP3(instruction),
             INSTRUCTION_FLAG_UNSIGNED(instruction),
             INSTRUCTION_FLAG_FLOAT(instruction));
      break;
    case MNEMONIC_DIV:
      BIN_OP(ctx, /, INSTRUCTION_OP1(instruction), INSTRUCTION_OP2(instruction),
             INSTRUCTION_OP3(instruction),
             INSTRUCTION_FLAG_UNSIGNED(instruction),
             INSTRUCTION_FLAG_FLOAT(instruction));
      break;
    case MNEMONIC_LOAD_CONST: {
      if (INSTRUCTION_FLAG_B32(instruction)) {
        uint32_t c = ctx->ip[1];
        if(INSTRUCTION_FLAG_SIGN_EXT(instruction)) {
          ctx->regs[INSTRUCTION_OP1(instruction)].int_num = (int32_t)c;
        }
        else {
          ctx->regs[INSTRUCTION_OP1(instruction)].int_num = c;
        }
        ctx->ip++;
      } else if (INSTRUCTION_FLAG_B64(instruction)) {
        uint64_t c;
        memcpy(&c, ctx->ip + 1, 8);
        ctx->regs[INSTRUCTION_OP1(instruction)].int_num = c;
        ctx->ip += 2;
      } else {
        return EXIT_MISSING_FLAG;
      }
    } break;
    }

    ctx->ip++;
  }

exit_loop:;
  return EXIT_OK;
}
