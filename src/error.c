#include "error.h"

#include <stdarg.h>
#include <stdio.h>

void print_err(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputc('\n', stderr);
}
