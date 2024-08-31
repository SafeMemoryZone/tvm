#include "common.h"
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

void print_err(char *fmt, ...) {
  va_list args_ptr;
  va_start(args_ptr, fmt);
  vfprintf(stderr, fmt, args_ptr);
  fprintf(stderr, " (errno = %d)\n", errno);
  va_end(args_ptr);
}

char *err_as_str(int err_code) {
  static char* errors[] = {
    "ok",
    "error",
    "nothing to return",
  };

  assert((unsigned long) err_code < sizeof(errors) / sizeof(char*));
  return errors[err_code];
}
