
#include "mode.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
dbquery_usage (void)
{
    fprintf (stderr, "Usage: lpkg dbquery [-yn] sql_stmt\n");
    exit (EXIT_FAILURE);
}

int
lpkg_dbquery_main (int argc, char **argv)
{
    int ch, yflag;

    while ((ch = getopt (argc, argv, "+yn")) != -1)
    {
        switch (ch)
        {
        case 'y': yflag = 1; break;
        case 'n': yflag = 0; break;

        default:
            dbquery_usage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc <= 0)
    {
        dbquery_usage ();
    }

    fprintf (stderr, "unimplmemented dbquery\n");

    return 0;
}


/* end of file */
