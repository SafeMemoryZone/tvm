#ifndef ERROR_H
#define ERROR_H
#include <stdbool.h>
#include <stdio.h>

enum RetCode {
  RET_CODE_OK = 0,
  RET_CODE_ERR,
  RET_CODE_NORET,
};

#define ERR_IF(_cond, _fmt, ...)            \
  do {                                      \
    if (_cond) {                            \
      fprintf(stderr, _fmt, ##__VA_ARGS__); \
      fputc('\n', stderr);                  \
      return RET_CODE_ERR;                  \
    }                                       \
  } while (0)

void print_err(char *fmt, ...);
#endif  // ERROR_H
