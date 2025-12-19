
#include "lpkg.h"

#include "default.h"
#include "mode.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int g_vflag = DEFAULT_VFLAG;
char *g_dbfile = DEFAULT_DBFILE;

static void
usage (void)
{
    (void)fprintf (stderr, "Usage: lpkg [-tv] (autoremove|dbquery|info|init|"
                           "install|list|remove|update) [mode_arg...]\n");
    exit (EXIT_FAILURE);
}

int
lpkg_main (int argc, char **argv)
{
    char *mode = NULL;
    int ch;

    while ((ch = getopt (argc, argv, "+f:tv")) != -1)
    {
        switch (ch)
        {
        case 'f': g_dbfile = optarg; break;
        case 't': g_vflag = 0; break;
        case 'v': g_vflag = 1; break;

        default:
            usage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0)
    {
        usage ();
    }

    mode = *argv;

    if (0 == strcmp (mode, "autoremove"))
    {
        return lpkg_autoremove_main (argc, argv);
    }
    else if (0 == strcmp (mode, "dbquery"))
    {
        return lpkg_dbquery_main (argc, argv);
    }
    else if (0 == strcmp (mode, "info"))
    {
        return lpkg_info_main (argc, argv);
    }
    else if (0 == strcmp (mode, "init"))
    {
        return lpkg_init_main (argc, argv);
    }
    else if (0 == strcmp (mode, "install"))
    {
        return lpkg_install_main (argc, argv);
    }
    else if (0 == strcmp (mode, "list"))
    {
        return lpkg_list_main (argc, argv);
    }
    else if (0 == strcmp (mode, "remove"))
    {
        return lpkg_remove_main (argc, argv);
    }
    else if (0 == strcmp (mode, "update"))
    {
        return lpkg_update_main (argc, argv);
    }
    else
    {
        usage ();
    }

    /* UNREACHABLE */
    abort ();
}


/* end of file */
