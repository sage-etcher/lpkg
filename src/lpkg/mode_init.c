
#include "mode.h"

#include "conio.h"
#include "dbio.h"
#include "fileio.h"
#include "lpkg.h"

#include <sqlite3.h>

#include <assert.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static void init_usage (void);
static int pkgdb_init (int yflag);

static void
init_usage (void)
{
    (void)fprintf (stderr, "Usage: lpkg init [-y]\n");
    exit (EXIT_FAILURE);
}

int
lpkg_init_main (int argc, char **argv)
{
    int ch;
    int yflag = 0;

    optind = 1;
    while ((ch = getopt (argc, argv, "+y")) != -1)
    {
        switch (ch)
        {
        case 'y': yflag = 1; break;
        default:
            init_usage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (pkgdb_init (yflag))
    {
        fprintf (stderr, "error: failed to initialize package database\n");
        exit (EXIT_FAILURE);
    }

    return 0;
}

static int
pkgdb_init (int yflag)
{
    sqlite3 *db = NULL;
    int overwrite_flag = 0;

    /* if file exists, prompt user for confirmation */
    if (file_exists (g_dbfile))
    {
        printf ("database exists (%s)... ", g_dbfile);
        if (!yflag)
        {
            if ('y' != prompt ("overwrite it? [yN]: "))
            {
                printf ("cancelling database initialization\n");
                return -1;
            }
        }

        printf ("overwriting the old database file!\n");
        overwrite_flag = 1;

        if (remove (g_dbfile))
        {
            fprintf (stderr, "error: failed to remove file - %s\n", g_dbfile);
            return -1;
        }
    }
    /* open the database */
    db = db_open (g_dbfile);
    if (db == NULL) return -1;

    /* create tables */
    if (db_init_tables (db))
    {
        db_close (db);
        return -1;
    }

    /* log the transaction */
    if (overwrite_flag && db_transaction (db, "previous database overwritten."))
    {
        db_close (db);
        return -1;
    }

    /* log success */
    printf ("database successfully initialized - %s\n", g_dbfile);

    /* close the database */
    db_close (db);

    return 0;
}


/* end of file */
