#ifndef COMMON_H
#define COMMON_H

enum ErrCode {
  ERR_CODE_OK = 0,
  ERR_CODE_FS,
  ERR_CODE_ARGS,
  ERR_CODE_FILE_FORMAT,
  ERR_CODE_INVALID_IP,
  ERR_CODE_UNKNOWN_OPCODE,
  ERR_CODE_EOF,
  ERR_CODE_UNKNOW_TOK,
  ERR_CODE_OVERFLOW,
  ERR_CODE_UNDERFLOW,
};

void print_err(char *fmt, ...);
char *err_as_str(int err_code);
#endif // COMMON_H
