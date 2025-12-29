
#include "version.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static unsigned
atou_delim (const char *x, int delim)
{
    int value = 0;

    while ((*x != delim) && (*x != '\n'))
    {
        assert (isdigit (*x));
        value = *x - '0';
        x++;
    }

    return value;
}


/* verry basic version comparison
 * compares major.minor.patch versioning scheme */
int
version_cmp (const char *x, const char *y)
{
    const char *x_iter = x;
    const char *y_iter = y;

    int major = 0;
    int minor = 0;
    int patch = 0;

    major = atou_delim (x_iter, '.') - atou_delim (y_iter, '.');
    if (major != 0) return major;

    x_iter = strchr (x_iter, '.');
    y_iter = strchr (y_iter, '.');
    minor = atou_delim (x_iter, '.') - atou_delim (y_iter, '.');
    if (minor != 0) return minor;

    x_iter = strchr (x_iter, '.');
    y_iter = strchr (y_iter, '.');
    patch = atou_delim (x_iter, '.') - atou_delim (y_iter, '.');
    if (patch != 0) return patch;

    return 0;
}

/* end of file */
