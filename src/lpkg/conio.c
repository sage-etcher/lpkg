
#include "conio.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


char
vpromptf (const char *prompt_fmt, va_list args)
{
    char *line = NULL;
    size_t line_size = 0;
    int c = 0;

    vfprintf (stdout, prompt_fmt, args);
    fflush (stdout);

    (void)getline (&line, &line_size, stdin);
    assert (line != NULL);

    c = tolower (*line);

    free (line);
    line = NULL;
    line_size = 0;

    return c;
}

char
promptf (const char *prompt_fmt, ...)
{
    char c = 0;
    va_list args;

    va_start (args, prompt_fmt);
    c = vpromptf (prompt_fmt, args);

    va_end (args);
    return c;
}

char
prompt (const char *prompt_str)
{
    return promptf (prompt_str);
}

/* end of file */
