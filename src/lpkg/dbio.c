
#include "dbio.h"

#include "dbmap.h"
#include "fileio.h"
#include "package.h"

#include <sqlite3.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define SQLITE_OPEN_READWRITECREATE (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)

#define SQLITE_BIND_WRAP(TYPE,STMT,NAME,...) do {                           \
        int param_index = 0;                                                \
                                                                            \
        /* fprintf (stderr, "wrap: %s\n", NAME); */                         \
        param_index = sqlite3_bind_parameter_index (STMT, NAME);            \
        if (param_index == 0)                                               \
        {                                                                   \
            return -1;                                                      \
        }                                                                   \
                                                                            \
        return sqlite3_bind_ ## TYPE (STMT, param_index, __VA_ARGS__);      \
    } while (0)

static int db_insert_transaction (sqlite3 *db, const char *msg,
        const char *user, const char *date, time_t unix_time);

static const char *strchr_reverse (const char *str, int c);


static const char *
strchr_reverse (const char *str, int c)
{
    size_t n = 0;
    const char *iter = NULL;

    assert (str != NULL);

    n = strlen (str);

    iter = str + n;
    for (; iter >= str; iter--)
    {
        if (*iter == c)
        {
            return iter;
        }
    }

    return NULL;
}

sqlite3 *
db_open (const char *filepath)
{
    sqlite3 *db = NULL;
    const char *basename1 = NULL;
    ptrdiff_t dirname_len = 0;

    basename1 = strchr_reverse (filepath, '/');
    if (basename1 != NULL)
    {
        dirname_len = basename1 - filepath;
        (void)mkdirn_p (filepath, dirname_len);
    }

    if (sqlite3_open_v2 (filepath, &db, SQLITE_OPEN_READWRITECREATE, NULL))
    {
        fprintf (stderr, "error: failed to open - %s: %s\n", 
                filepath, sqlite3_errmsg (db));
        db_close (db);
        return NULL;
    }

    return db;
}

int
db_close (sqlite3 *db)
{
    int remaining_retrys = 3;

    while (sqlite3_close (db))
    {
        fprintf (stderr, "error: failed to close database: %s\n", 
                sqlite3_errmsg (db));

        remaining_retrys--;
        if (remaining_retrys <= 0) 
        {
            fprintf (stderr, "error: max retry's reached, unsafe abort!\n"); 
            break;
        }
    }

    return (remaining_retrys > 0);
}

int
db_init_tables (sqlite3 *db)
{
    const char DB_CREATE_TABLES[] = { 
        /* {{{ */
        "CREATE TABLE transaction_log "
            "(transaction_id INTEGER PRIMARY KEY ASC AUTOINCREMENT"
            ",user TEXT NOT NULL"
            ",date TEXT NOT NULL"
            ",unix_time INTEGER NOT NULL"
            ",description TEXT NOT NULL);"

        "CREATE TABLE package "
            "(package_id INTEGER PRIMARY KEY ASC AUTOINCREMENT"
            ",name TEXT NOT NULL"
            ",version TEXT NOT NULL"
            ",license TEXT"
            ",homepage TEXT"
            ",pkg_revision INTEGER NOT NULL"
            ",pkg_maintainer_name TEXT"
            ",pkg_maintainer_email TEXT"
            ",automatic BOOLEAN NOT NULL"
            ",active BOOLEAN NOT NULL);"

        "CREATE TABLE file_ownership "
            "(file_id INTEGER PRIMARY KEY ASC AUTOINCREMENT"
            ",package_id REFERENCES package"
            ",path TEXT NOT NULL"
            ",checksum TEXT"
            ",active BOOLEAN NOT NULL);"

        "CREATE TABLE dependency "
            "(dependency_id INTEGER PRIMARY KEY ASC AUTOINCREMENT"
            ",package_id REFERENCES package"
            ",dep_name TEXT NOT NULL"
            ",dep_ver_min STRING"
            ",dep_ver_max STRING"
            ",active BOOLEAN NOT NULL);"
    }; /* }}} */
    char *sql_errmsg = NULL;

    /* create the tables */
    if (sqlite3_exec (db, DB_CREATE_TABLES, NULL, NULL, &sql_errmsg))
    {
        fprintf (stderr, "error: failed to execute SQL statement: %s\n", 
                sqlite3_errmsg (db));
        sqlite3_free (sql_errmsg);
        return -1;
    }

    /* log the transaction */
    return db_transaction (db, "initialized database.");
}

static int 
db_insert_transaction (sqlite3 *db, const char *msg, const char *user, 
        const char *date, time_t unix_time)
{
    int rc = -1;

    const char TRANSACTION_LOG[] = {
        "INSERT INTO transaction_log (user,date,unix_time,description) "
        "VALUES (@user, @date, @unix_time, @msg);"
    };
    const char *tail = NULL;
    sqlite3_stmt *stmt = NULL;
    sql_map_t *bind_args = NULL;

    if (sqlite3_prepare_v2 (db, TRANSACTION_LOG, -1, &stmt, &tail))
    {
        fprintf (stderr, "error: failure preparing log statement\n");
        goto cleanup;
    }

    shput (bind_args, "@user",      SQLVAL_S ((char *)user));
    shput (bind_args, "@msg",       SQLVAL_S ((char *)msg));
    shput (bind_args, "@date",      SQLVAL_S ((char *)date));
    shput (bind_args, "@unix_time", SQLVAL_I (unix_time));
    if ((rc = dbmap_bind (stmt, bind_args)))
    {
        fprintf (stderr, "error: failure binding to log statement: %s\n",
                sqlite3_errstr (rc));
        goto cleanup;
    }

    rc = sqlite3_step (stmt);
    switch (rc)
    {
    case SQLITE_DONE: rc = 0; break;
    default:
        fprintf (stderr, "error: failure executing log statement: %s\n",
                sqlite3_errstr (rc));
        break;
    }

cleanup:
    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);
    shfree (bind_args);

    return rc;
}

int
db_transaction (sqlite3 *db, const char *fmt, ...)
{
    va_list args, args_cpy;
    int rc = 0;
    const char *user = getenv ("USER");
    time_t unix_time = time (NULL);
    struct tm *tm = localtime (&unix_time);
    char *msg = NULL;
    int msg_n = 0;

    enum { DATE_MAX = 19 };
    char date[DATE_MAX+1] = { 0 };

    (void)strftime (date, DATE_MAX+1, "%Y-%m-%d %H:%M:%S", tm);

    va_start (args, fmt);
    va_start (args_cpy, fmt);
    msg_n = vsnprintf (NULL, 0, fmt, args);

    assert (msg_n > 0);
    msg = malloc (msg_n + 1);
    assert (msg != NULL);
    (void)vsnprintf (msg, msg_n + 1, fmt, args_cpy);

    va_end (args);
    va_end (args_cpy);

    rc = db_insert_transaction (db, msg, user, date, unix_time);

    free (msg);
    return rc;
}

int
db_bind_vvv_int (sqlite3_stmt *stmt, const char *name, int value)
    { SQLITE_BIND_WRAP (int, stmt, name, value); }

int
db_bind_vvv_text (sqlite3_stmt *stmt, const char *name, const char *text, 
        int text_length, void (*free_cb)(void *))
    { SQLITE_BIND_WRAP (text, stmt, name, text, text_length, free_cb); }

int
db_package_install (sqlite3 *db, package_t *p_pkg)
{
    const char INSERT_PACKAGE[] = {
        "INSERT INTO package (name, version, license, homepage, pkg_revision, "
                             "pkg_maintainer_name, pkg_maintainer_email, "
                             "automatic, active) "
        "VALUES (@name, @version, @license, @homepage, @revision, @maint_name,"
                "@maint_email, @automatic, True);"
    };
    int rc = 0;
    sql_map_t *bind_args = NULL;

    shput (bind_args, "@name",        SQLVAL_S (p_pkg->program_name));
    shput (bind_args, "@version",     SQLVAL_S (p_pkg->program_version));
    shput (bind_args, "@license",     SQLVAL_S (p_pkg->program_license));
    shput (bind_args, "@homepage",    SQLVAL_S (p_pkg->program_homepage));
    shput (bind_args, "@revision",    SQLVAL_I (p_pkg->package_revision));
    shput (bind_args, "@maint_name",  SQLVAL_S (p_pkg->maintainer_name));
    shput (bind_args, "@maint_email", SQLVAL_S (p_pkg->maintainer_email));
    shput (bind_args, "@automatic",   SQLVAL_I (p_pkg->package_automatic));

    rc = dbmap_execute (db, INSERT_PACKAGE, -1, bind_args);
    shfree (bind_args);
    bind_args = NULL;

    if (rc != SQLITE_DONE)
    {
        fprintf (stderr, "error: dbmap_execute: %d - %s\n",
                rc, sqlite3_errstr (rc));
        return rc;
    }

    return SQLITE_OK;
}

int
db_package_uninstall (sqlite3 *db, package_t *p_pkg)
{
    const char DEACTIVATE_PACKAGE[] = {
        "UPDATE package "
        "SET active = False "
        "WHERE package_id = @id;"
    };
    int rc = 0;
    sql_map_t *bind_args = NULL;

    shput (bind_args, "@id", SQLVAL_I (p_pkg->package_id));

    rc = dbmap_execute (db, DEACTIVATE_PACKAGE, -1, bind_args);
    shfree (bind_args);
    bind_args = NULL;

    if (rc != SQLITE_DONE)
    {
        fprintf (stderr, "error: dbmap_execute: %d - %s\n",
                rc, sqlite3_errstr (rc));
        return 1;
    }

    return 0;
}

int
db_get_package (sqlite3 *db, const char *name, package_t *p_ret_pkg)
{
    const char SELECT_PKG_BY_NAME[] = {
        "SELECT * "
        "FROM package "
        "WHERE "
            "name = @name AND "
            "active = True "
        "ORDER BY version DESC;"
    };
    int rc = 0;
    const char *tail = NULL;
    sqlite3_stmt *stmt = NULL;
    package_t pkg = { 0 };
    sql_map_t *row = NULL;

    package_init (&pkg);

    rc = sqlite3_prepare_v2 (db, SELECT_PKG_BY_NAME, -1, &stmt, &tail);
    if (rc != SQLITE_OK)
    {
        /* cleanup and exit w/ error */
        fprintf (stderr, "error: failed to prepare sql statement - %s\n",
                SELECT_PKG_BY_NAME);
        goto early_exit;
    }

    rc = db_bind_vvv_text (stmt, "@name", name, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK)
    {
        fprintf (stderr, "error: failed to bind parameter\n");
        goto early_exit;
    }

    rc = dbmap_step (stmt, &row);
    if (rc == SQLITE_ROW)
    {
        (void)package_from_map (&pkg, row);
    }
    else if (rc != SQLITE_DONE)
    {
        /* cleanup and exit w/ error */
        fprintf (stderr, "error: failed execute sql statement - %s\n",
                SELECT_PKG_BY_NAME);
        goto early_exit;
    }

early_exit:
    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);
    shfree (row);
    row = NULL;

    *p_ret_pkg = pkg;
    return rc;
}

/* vim: fdm=marker
 * end of file */

