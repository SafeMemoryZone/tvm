#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>

#define FILE_SIG_SIZE 3
#define FILE_SIG "TVM"
#define LIB_ERR_IF(cond, msg, ...)                                             \
  do {                                                                         \
    if (cond) {                                                                \
      print_err(msg, __VA_ARGS__);                                             \
      return RET_CODE_ERR;                                                     \
    }                                                                          \
  } while (0)
#define ERR_IF(cond, msg, ...)                                                 \
  do {                                                                         \
    if (cond) {                                                                \
      fprintf(stderr, msg, __VA_ARGS__);                                       \
      fputc('\n', stderr);                                                     \
      return RET_CODE_ERR;                                                     \
    }                                                                          \
  } while (0)

typedef uint32_t inst_ty;

enum RetCode {
  RET_CODE_OK = 0,
  RET_CODE_ERR,
  RET_CODE_NORET,
};

void print_err(char *fmt, ...);
char *err_as_str(int err_ret_code);
#endif // COMMON_H
