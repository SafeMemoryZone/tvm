#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal_error_v(char *format, va_list argptr) {
  vfprintf(stderr, format, argptr);
  exit(1);
}

void fatal_error(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  fatal_error_v(format, argptr);
  va_end(argptr);
}

void expect(int cond, char *format, ...) {
  if (!cond) {
    va_list argptr;
    va_start(argptr, format);
    fatal_error_v(format, argptr);
    va_end(argptr);
  }
}
