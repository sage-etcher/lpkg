
#include "dbmap.h"

#include <sqlite3.h>
#include <stb_ds.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int
dbmap_bind (sqlite3_stmt *stmt, sql_map_t *input)
{
    int rc = 0;
    int i = shlen (input);
    int index = 0;
    sql_value_t *p_val = NULL;

    assert (stmt != NULL);

    for (--i; i >= 0; i--)
    {
        index = sqlite3_bind_parameter_index (stmt, input[i].key);
        if (index <= 0)
        {
            sqlite3_reset (stmt);
            return SQLITE_ERROR;
        }

        p_val = &input[i].value;

        switch (p_val->sql_type)
        {
        case SQLITE_INTEGER: 
            /* printf ("bind %d = { \"%s\": %d }\n", i, input[i].key, p_val->m.i); */
            rc = sqlite3_bind_int (stmt, index, p_val->m.i); 
            break;

        case SQLITE_TEXT: 
            /* printf ("bind %d = { \"%s\": \"%s\" }\n", i, input[i].key, p_val->m.s); */
            rc = sqlite3_bind_text (stmt, index, p_val->m.s, -1, SQLITE_STATIC);
            break;

        default:
            fprintf (stderr, "error: dbmap_bind unhandled type - %s: %d: %s\n",
                    input[i].key, p_val->sql_type, 
                    sqlite3_column_decltype (stmt, index));
            sqlite3_reset (stmt);
            return SQLITE_ERROR;
        }

        if (rc != SQLITE_OK)
        {
            fprintf (stderr, "error: bind failed for arguement - %s\n", 
                    input[i].key);
            sqlite3_reset (stmt);
            return SQLITE_ERROR;
        }
    }

    return SQLITE_OK;
}

int
dbmap_step (sqlite3_stmt *stmt, sql_map_t **p_output)
{
    int i = 0;
    int col_count = 0;
    int col_type = 0;
    const char *col_name = NULL;

    int rc = 0;
    sql_map_t *output = NULL;

    assert (stmt != NULL);
    assert (p_output != NULL);

    rc = sqlite3_step (stmt);
    if (rc != SQLITE_ROW) return rc;

    col_count = sqlite3_column_count (stmt);
    if (col_count < 1) return rc;

    for (i = 0; i < col_count; i++)
    {
        col_name = sqlite3_column_name (stmt, i);
        col_type = sqlite3_column_type (stmt, i);

        switch (col_type)
        {
        case SQLITE_INTEGER:
            shput (output, col_name, SQLVAL_I (sqlite3_column_int (stmt, i)));
            break;

        case SQLITE_TEXT:
            shput (output, col_name, SQLVAL_S ((char *)sqlite3_column_text (stmt, i)));
            break;

        default:
            fprintf (stderr, "error: dbmap_step unhandled type - %s %d: %s\n",
                    col_name, col_type, sqlite3_column_decltype (stmt, i));
            return SQLITE_ERROR;
        }
    }

    *p_output = output;
    return rc;
}

int
dbmap_execute (sqlite3 *db, const char *sql_text, int n, sql_map_t *input)
{
    int rc = 0;
    sqlite3_stmt *stmt = NULL;

    assert (db != NULL);
    assert (sql_text != NULL);

    rc = sqlite3_prepare_v2 (db, sql_text, n, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf (stderr, "error: cannot prepare sqlite statment - %s\n",
                sql_text);
        goto early_exit;
    }

    if (input != NULL)
    {
        rc = dbmap_bind (stmt, input);
        if (rc != SQLITE_OK)
        {
            fprintf (stderr, "error: statement bind failure\n");
            goto early_exit;
        }
    }

    rc = sqlite3_step (stmt);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE))
    {
        fprintf (stderr, "error: statement step failure\n");
        goto early_exit;
    }

early_exit:
    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);
    return rc;
}


/* end of file */
