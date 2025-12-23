
#include "unpack.h"

#include <archive.h>
#include <archive_entry.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


uint8_t *
unpack_fread (const char *archive, const char *filepath)
{
    int r;
    struct archive *a;
    struct archive_entry *entry;

    const char *fpath;
    mode_t fmode;
    size_t fsize;

    char c;

    uint8_t *buf = NULL;
    int size;

    a = archive_read_new ();
    archive_read_support_filter_all (a);
    archive_read_support_format_all (a);

    r = archive_read_open_filename (a, archive, 10240);
    if (r != ARCHIVE_OK)
    {
        fprintf (stderr, "error #1\n");
        goto exit;
    }

    while (archive_read_next_header (a, &entry) == ARCHIVE_OK)
    {
        fmode = archive_entry_filetype (entry);

        if (S_ISDIR (fmode)) continue;

        fpath = archive_entry_pathname (entry);
        for (; *fpath == '.' || *fpath == '/'; fpath++) {}

        if (0 != strcmp (filepath, fpath)) continue;

        fsize = archive_entry_size (entry);
        assert (fsize != 0);

        buf = malloc (fsize+1);
        assert (buf != NULL);

        size = archive_read_data (a, buf, fsize);
        if (size < 0)
        {
            fprintf (stderr, "error: aaaaaaaaaaaa\n");
            free (buf);
            buf = NULL;
        }

        break;
    }

exit:
    r = archive_read_free (a);
    if (r != ARCHIVE_OK)
    {
        fprintf (stderr, "error #2\n");
        free (buf);
        buf = NULL;
    }

    return buf;
}

int
unpack_extract (const char *archive, const char *path, const char *destdir)
{
    return 0;
}

/* end of file */
