#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>

#define TVM_FILE_SIGNATURE_SIZE 3
#define TVM_FILE_SIGNATURE "TVM"

typedef uint32_t inst_ty;

enum RetCode {
  RET_CODE_OK = 0,
  RET_CODE_ERR,
  RET_CODE_NORET,
};

void print_err(char *fmt, ...);
char *err_as_str(int err_ret_code);
#endif // COMMON_H
