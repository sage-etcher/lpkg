
#ifndef LPKG_CONIO_H
#define LPKG_CONIO_H

#include <stdarg.h>

char vpromptf (const char *prompt_fmt, va_list args);
char promptf (const char *prompt_fmt, ...);
char prompt (const char *prompt_str);

#endif
/* end of file */
