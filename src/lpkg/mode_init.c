
#include "mode.h"

#include "lpkg.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int pkgdb_init (int fflag);

static void
init_usage (void)
{
    (void)fprintf (stderr, "Usage: lpkg init [-f]\n");
    exit (EXIT_FAILURE);
}

int
lpkg_init_main (int argc, char **argv)
{
    int ch, fflag;

    optind = 1;
    while ((ch = getopt (argc, argv, "+f")) != -1)
    {
        switch (ch)
        {
        case 'f': fflag = 1; break;

        default:
            init_usage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (pkgdb_init (fflag))
    {
        fprintf (stderr, "error: failed to initialize package database\n");
        exit (EXIT_FAILURE);
    }

    return 0;
}

static int
pkgdb_init (int fflag)
{
    fprintf (stderr, "unimplmented pkg_init\n");
    return 1;
}


/* end of file */
