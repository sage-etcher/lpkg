
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

    int i = 0;
    int n = 0;
    const char *column_name = NULL;

    int index = 0;
    sql_value_t *p_val = NULL;

    assert (stmt != NULL);

    n = sqlite3_bind_parameter_count (stmt);
    for (i = 0; i < n; i++)
    {
        /* get column name */
        column_name = sqlite3_bind_parameter_name (stmt, i);

        /* get mapped value */
        index = shgeti (input, column_name);
        if (index == -1)
        {
            fprintf (stderr, "error: dbmap_bind missing column - %s\n",
                    column_name);
            sqlite3_reset (stmt);
            return SQLITE_ERROR;
        }

        p_val = &input[index].value;

        /* bind by value type */
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

        /* log error for failed binds */
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
dbmap_prepare_v2 (sqlite3 *db, const char *sql_text, int n, sql_map_t *input, 
        sqlite3_stmt **p_stmt)
{
    int rc = 0;
    sqlite3_stmt *stmt = NULL;

    assert (db != NULL);
    assert (sql_text != NULL);
    assert (p_stmt != NULL);

    rc = sqlite3_prepare_v2 (db, sql_text, n, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf (stderr, "error: cannot prepare sqlite statment - %s\n",
                sql_text);
        return rc;
    }

    if (input != NULL)
    {
        rc = dbmap_bind (stmt, input);
        if (rc != SQLITE_OK)
        {
            fprintf (stderr, "error: statement bind failure\n");
            return rc;
        }
    }

    *p_stmt = stmt;
    return rc;
}


int
dbmap_execute (sqlite3 *db, const char *sql_text, int n, sql_map_t *input)
{
    int rc = 0;
    sqlite3_stmt *stmt = NULL;

    assert (db != NULL);
    assert (sql_text != NULL);

    rc = dbmap_prepare_v2 (db, sql_text, n, input, &stmt);
    if (rc != SQLITE_OK) return rc;


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

int
dbmap_execute_v2 (sqlite3 *db, const char *sql, sql_map_t *input, 
        int (*callback)(void *, sql_map_t *), void *user)
{
    int sql_rc = 0;
    sqlite3_stmt *stmt = NULL;
    sql_map_t *output;

    assert (db  != NULL);
    assert (sql != NULL);

    do {
        /* prepare statement in `sql`, return next statement in `sql` */
        sql_rc = sqlite3_prepare_v2 (db, sql, -1, &stmt, &sql);
        if (sql_rc != SQLITE_OK)
        {
            fprintf (stderr, "error: failure to prepare SQL statement - %s\n",
                    sqlite3_errstr (sql_rc));
            goto early_exit;
        }

        /* bind parameters to statement */
        sql_rc = dbmap_bind (stmt, input);
        if (SQLITE_OK != sql_rc) goto early_exit;

        /* execute the statement */
        while (SQLITE_ROW == (sql_rc = dbmap_step (stmt, &output)))
        {
            if (callback == NULL) continue;
            if (callback (user, output))
            {
                fprintf (stderr, "warning: callback cancelled\n");
                goto early_exit;
            }
        }

        if (sql_rc != SQLITE_DONE)
        {
            fprintf (stderr, "error: failure to step SQL statement - %s\n",
                    sqlite3_errstr (sql_rc));
            goto early_exit;
        }

        /* destroy the statement */
    early_exit:
        sqlite3_reset (stmt);
        sqlite3_finalize (stmt);

        shfree (output);
        output = NULL;
    } while ((sql_rc == SQLITE_DONE) && (sql != NULL) && (*sql != '\0'));

    return sql_rc;
}


/* end of file */
