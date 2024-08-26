#include "vm.h"
#include <stdint.h>
#include <string.h>

#define INST_MNEMONIC(inst) ((inst) & 0xFF)
#define INST_OP1(inst) (((inst) >> 8) & 0b111)
#define INST_OP2(inst) (((inst) >> 11) & 0b111)
#define INST_OP3(inst) (((inst) >> 14) & 0b111)

#define INST_FLAG_UNSIGNED(inst) (((inst) >> 17) & 1)
#define INST_FLAG_FLOAT(inst) (((inst) >> 18) & 1)
#define INST_FLAG_32(inst) (((inst) >> 19) & 1)
#define INST_FLAG_64(inst) (((inst) >> 20) & 1)

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

void vm_init_ctx(VmCtx *ctx, uint32_t *insts, size_t insts_size) {
  ctx->insts = insts;
  ctx->insts_size = insts_size;
  ctx->ip = insts;
}

int vm_run(VmCtx *ctx) {
  for (;;) {
    if (ctx->ip >= ctx->insts + ctx->insts_size)
      return EXIT_INVALID_IP;

    uint32_t inst = *ctx->ip;

    switch (INST_MNEMONIC(inst)) {
    default:
      return EXIT_INVALID_inst;
    case MNEMONIC_HALT:
      goto exit_loop;
    case MNEMONIC_ADD:
      BIN_OP(ctx, +, INST_OP1(inst), INST_OP2(inst),
             INST_OP3(inst),
             INST_FLAG_UNSIGNED(inst),
             INST_FLAG_FLOAT(inst));
      break;
    case MNEMONIC_SUB:
      BIN_OP(ctx, -, INST_OP1(inst), INST_OP2(inst),
             INST_OP3(inst),
             INST_FLAG_UNSIGNED(inst),
             INST_FLAG_FLOAT(inst));
      break;
    case MNEMONIC_MUL:
      BIN_OP(ctx, *, INST_OP1(inst), INST_OP2(inst),
             INST_OP3(inst),
             INST_FLAG_UNSIGNED(inst),
             INST_FLAG_FLOAT(inst));
      break;
    case MNEMONIC_DIV:
      BIN_OP(ctx, /, INST_OP1(inst), INST_OP2(inst),
             INST_OP3(inst),
             INST_FLAG_UNSIGNED(inst),
             INST_FLAG_FLOAT(inst));
      break;
    case MNEMONIC_LOAD_CONST: {
      // remove reliance on endianess (use htonll)
      if (INST_FLAG_32(inst)) {
        uint32_t c = ctx->ip[1];
        if(!INST_FLAG_UNSIGNED(inst))
          ctx->regs[INST_OP1(inst)].int_num = (int32_t)c;
        else
          ctx->regs[INST_OP1(inst)].int_num = c;

        ctx->ip++;
      } else if (INST_FLAG_64(inst)) {
        uint64_t c;
        memcpy(&c, ctx->ip + 1, 8);
        ctx->regs[INST_OP1(inst)].int_num = c;
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


char *vm_stringify_exit_code(int code) {
  switch(code) {
    default: return "Ok";
    case EXIT_INVALID_IP:
      return "Invalid ip";
    case EXIT_INVALID_inst:
      return "Invalid inst";
    case EXIT_MISSING_FLAG:
      return "Missing inst flag";
  }
}
