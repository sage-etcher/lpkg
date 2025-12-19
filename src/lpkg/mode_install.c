
#include "mode.h"

#include "lpkg.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int pkg_install (char *pkg_name, int fflag);

static void
install_usage (void)
{
    (void)fprintf (stderr, "Usage: lpkg install [-f] package [package...]\n");
    exit (EXIT_FAILURE);
}

int
lpkg_install_main (int argc, char **argv)
{
    int ch, fflag;

    optind = 1;
    while ((ch = getopt (argc, argv, "+f")) != -1)
    {
        switch (ch)
        {
        case 'f': fflag = 1; break;

        default:
            install_usage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0)
    {
        install_usage ();
    }

    while (argc > 0)
    {
        if (pkg_install (*argv, fflag))
        {
            fprintf (stderr, "error: failed to install package - %s\n", *argv);
            exit (EXIT_FAILURE);
        }

        argc--;
        argv++;
    }

    return 0;
}

static int
pkg_install (char *pkg_name, int fflag)
{
    fprintf (stderr, "unimplmented pkg_install\n");
    return 1;
}


/* end of file */
