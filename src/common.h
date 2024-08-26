#ifndef COMMON_H
#define COMMON_H
#include <stdarg.h>

void fatal_err_v(char *format, va_list argptr);
void fatal_err(char *format, ...);
void expect(int cond, char *format, ...);
#endif // COMMON_H
