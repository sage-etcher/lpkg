
#include "mode.h"

#include "conio.h"
#include "dbio.h"
#include "lpkg.h"

#include <sqlite3.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int query_database (char *filepath, char *stmt_text);

static void
dbquery_usage (void)
{
    fprintf (stderr, "Usage: lpkg dbquery [-y] sql_stmt\n");
    exit (EXIT_FAILURE);
}

int
lpkg_dbquery_main (int argc, char **argv)
{
    int ch;
    int yflag = 0;

    while ((ch = getopt (argc, argv, "+yn")) != -1)
    {
        switch (ch)
        {
        case 'y': yflag = 1; break;

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

    if (!yflag)
    {
        printf ("WARNIING: dbquery gives you raw access to the package "
                "database. This can verry easily brick your database.\n");
        if ('y' != prompt ("Would you like to continue? [yN]: "))
        {
            printf ("Cancelling raw database query\n");
            return -1;
        }
    }

    return query_database (g_dbfile, *argv);
}

static int
query_database (char *filepath, char *stmt_text)
{
    sqlite3 *db = NULL;
    const char *tail = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc = 0;

    db = db_open (filepath);
    if (db == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2 (db, stmt_text, -1, &stmt, &tail))
    {
        fprintf (stderr, "error: failed to prepare statement: %s\n",
                sqlite3_errmsg (db));
        db_close (db);
        return -1;
    }

    while ((rc = sqlite3_step (stmt)))
    {
        /* log results to stdout */
    }

    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);
    db_close (db);
    return 0;
}


/* end of file */
