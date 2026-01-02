
#include "mode.h"

#include "conio.h"
#include "dbio.h"
#include "lpkg.h"
#include "package.h"

#include <sqlite3.h>

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int pkg_remove (char *pkg, int yflag);

static void
remove_usage (void)
{
    (void)fprintf (stderr, "Usage: lpkg remove [-y] package [package...]\n");
    exit (EXIT_FAILURE);
}

int
lpkg_remove_main (int argc, char **argv)
{
    int ch, yflag = 0;

    optind = 1;
    while ((ch = getopt (argc, argv, "+y")) != -1)
    {
        switch (ch)
        {
        case 'y': yflag = 1; break;

        default:
            remove_usage ();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc == 0)
    {
        remove_usage ();
    }

    while (argc > 0)
    {
        if (pkg_remove (*argv, yflag))
        {
            fprintf (stderr, "error: failed to remove package - %s\n", *argv);
            exit (EXIT_FAILURE);
        }

        argc--;
        argv++;
    }

    return 0;
}

static int
pkg_remove (char *pkg_name, int yflag)
{
    sqlite3 *db = NULL;
    package_t pkg = { 0 };

    db = db_open (g_dbfile);
    if (db == NULL)
    {
        fprintf (stderr, "error: failed to open database\n");
        return -1;
    }

    if (db_package_get (db, pkg_name, &pkg))
    {
        fprintf (stderr, "error: failed to find any package named - %s\n",
                pkg_name);
        db_close (db);
        return -1;
    }

    if (db_package_uninstall (db, &pkg))
    {
        fprintf (stderr, "error: failed to remove package\n");
        package_free (&pkg);
        db_close (db);
        return -1;
    }

    package_free (&pkg);
    return 0;
}


/* end of file */
