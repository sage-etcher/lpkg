
#include "fileio.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


int
file_exists (const char *filepath)
{
    FILE *fp = fopen (filepath, "r");
    if (fp == NULL) return 0;

    (void)fclose (fp);
    return 1;
}

int
mkdirn_p (const char *path, size_t n)
{
    int rc = 0;
    char ch_copy = 0;
    char *path_copy = NULL;
    char *iter = NULL;

    assert (path != NULL);

    /* create a read/writeable path */
    path_copy = malloc (n + 1);
    assert (path_copy != NULL);
    memcpy (path_copy, path, n);
    path_copy[n] = '\0';

    /* create each subdir */
    iter = path_copy;
    while (NULL != (iter = strchr (iter, '/')))
    {
        ch_copy = *iter;
        *iter = '\0';

        mkdir (path_copy, 0755);

        *iter++ = ch_copy;
    }

    mkdir (path_copy, 0755);

    free (path_copy);
    return rc;
}
/* end of file */
