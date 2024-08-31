#ifndef COMMON_H
#define COMMON_H

enum ReturnCode {
  RETURN_CODE_OK = 0,
  RETURN_CODE_ERR,
  RETURN_CODE_NORETURN,
};

void print_err(char *fmt, ...);
char *err_as_str(int err_code);
#endif // COMMON_H
