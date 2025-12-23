
#include "mode.h"

#include "conio.h"
#include "dbio.h"
#include "lpkg.h"

#include <sqlite3.h>

#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX(x,y) ((x) < (y) ? (y) : (x));

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

    optind = 1;
    while ((ch = getopt (argc, argv, "+y")) != -1)
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

    if (argc < 0)
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

    int i = 0;
    int j = 0;
    const char *tmps = NULL;
    void *tmp = NULL;

    int col_count = 0;
    size_t *col_widths = NULL;
    char **col_values = NULL;

    size_t row_alloc = 0;
    size_t row_count = 0;


    db = db_open (filepath);
    if (db == NULL)
    {
        return -1;
    }

    db_transaction (db, "(unsafe) dbquery statement: %s", stmt_text);

    if (SQLITE_OK != (rc = sqlite3_prepare_v2 (db, stmt_text, -1, &stmt, &tail)))
    {
        printf ("stmt: '''%s'''\n", stmt_text);
        fprintf (stderr, "error: failed to prepare statement: %s\n",
                sqlite3_errstr (rc));

        db_transaction (db, "failed to prepare statement: %s", 
                sqlite3_errstr (rc));
        db_close (db);
        return -1;
    }

    col_count = sqlite3_column_count (stmt);
    assert (col_count > 0);

    col_widths = malloc (col_count * sizeof (*col_widths));
    col_values = malloc (col_count * sizeof (*col_values));
    assert (col_widths != NULL);
    assert (col_values != NULL);
    row_alloc = 1;

    for (i = 0; i < col_count; i++)
    {
        tmps = (const char *)sqlite3_column_name (stmt, i);
        col_values[i] = strdup (tmps);
        col_widths[i] = strlen (tmps);
    }
    row_count++;

    while (SQLITE_DONE != (rc = sqlite3_step (stmt)))
    {
        if (rc != SQLITE_ROW)
        {
            fprintf (stderr, "error: sqlite_step() returned error: %s\n",
                    sqlite3_errstr (rc));
            break;
        }

        while (row_alloc <= row_count)
        {
            tmp = realloc (col_values, col_count * row_alloc * 2 * sizeof (*col_values));
            assert (tmps != NULL);
            col_values = tmp;
            row_alloc *= 2;
        }

        for (i = 0; i < col_count; i++)
        {
            tmps = (const char *)sqlite3_column_text (stmt, i);
            if (tmps == NULL)
            {
                tmps = "NULL";
            }
            col_values[row_count*col_count+i] = strdup (tmps);
            col_widths[i] = MAX (col_widths[i], strlen (tmps));
        }
        row_count++;
    }

    for (i = 0; i < (int)row_count; i++)
    {
        for (j = 0; j < (int)col_count; j++)
        {
            printf (" %-*s |", (int)col_widths[j], col_values[i*col_count+j]);
        }
        printf ("\n");
    }

    for (i = 0; i < (int)row_count; i++)
    {
        free (col_values[i]);
    }
    free (col_values); 
    free (col_widths); 

    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);
    db_close (db);
    return 0;
}


/* end of file */
