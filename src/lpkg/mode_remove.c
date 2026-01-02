
#include "mode.h"

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
    const char DEACTIVATE_PACKAGE[] = {
        "UPDATE package "
        "SET active = False "
        "WHERE package_id = @package_id AND "
              "active = True;"
    };
    int rc = 1;
    sqlite3 *db = NULL;
    package_t pkg = { 0 };
    sql_map_t *input = NULL;

    db = db_open (g_dbfile);
    if (db == NULL)
    {
        fprintf (stderr, "error: failed to open database\n");
        goto early_exit;
    }

    if (db_package_get (db, pkg_name, &pkg))
    {
        fprintf (stderr, "error: failed to find any package named - %s\n",
                pkg_name);
        goto early_exit;
    }

    input = package_to_map (&pkg);
    if (dbmap_execute_v2 (db, DEACTIVATE_PACKAGE, input, NULL, NULL))
    {
        fprintf (stderr, "error: failed to remove package\n");
        goto early_exit;
    }

    rc = 0;
early_exit:
    shfree (input);
    package_free (&pkg);
    db_close (db);
    return rc;
}


/* end of file */
