
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


/* expect that `expr` return true, return a success as 0;
 * while if `expr` returns false, return an error as != 0. */
#define EXPECT(expr) (!(expr))


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
            ",active BOOLEAN NOT NULL"
            ",UNIQUE(name, version, pkg_revision));"

        "CREATE TABLE file_ownership "
            "(file_id INTEGER PRIMARY KEY ASC AUTOINCREMENT"
            ",package_id REFERENCES package"
            ",path TEXT NOT NULL"
            ",checksum TEXT"
            ",active BOOLEAN NOT NULL"
            ",UNIQUE(active, path));"

        "CREATE TABLE dependency "
            "(dependency_id INTEGER PRIMARY KEY ASC AUTOINCREMENT"
            ",package_id REFERENCES package"
            ",dep_name TEXT NOT NULL"
            ",dep_ver_min STRING"
            ",dep_ver_max STRING"
            ",active BOOLEAN NOT NULL"
            ",UNIQUE(active, package_id, dep_name));"
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

    (void)db_transaction (db, "initialized database.");

    /* log the transaction */
    return 0;
}

static int 
db_insert_transaction (sqlite3 *db, const char *msg, const char *user, 
        const char *date, time_t unix_time)
{
    int sql_rc = -1;

    const char TRANSACTION_LOG[] = {
        "INSERT INTO transaction_log (user,date,unix_time,description) "
        "VALUES (@user, @date, @unix_time, @msg);"
    };
    const char *tail = NULL;
    sqlite3_stmt *stmt = NULL;
    sql_map_t *input = NULL;

    if (sqlite3_prepare_v2 (db, TRANSACTION_LOG, -1, &stmt, &tail))
    {
        fprintf (stderr, "error: failure preparing log statement\n");
        goto cleanup;
    }

    shput (input, "@user",      SQLVAL_S ((char *)user));
    shput (input, "@msg",       SQLVAL_S ((char *)msg));
    shput (input, "@date",      SQLVAL_S ((char *)date));
    shput (input, "@unix_time", SQLVAL_I (unix_time));
    if ((sql_rc = dbmap_bind (stmt, input)))
    {
        fprintf (stderr, "error: failure binding to log statement: %s\n",
                sqlite3_errstr (sql_rc));
        goto cleanup;
    }

    sql_rc = sqlite3_step (stmt);
    if (sql_rc != SQLITE_DONE)
    {
        fprintf (stderr, "error: failure executing log statement: %s\n",
                sqlite3_errstr (sql_rc));
    }

cleanup:
    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);
    shfree (input);

    return EXPECT (sql_rc == SQLITE_DONE);
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
db_package_select (sqlite3 *db, package_t *pkg, package_t *p_match)
{
    const char SELECT_PACKAGE[] = {
        "SELECT * "
        "FROM package "
        "WHERE name         = @name     AND "
              "version      = @version  AND "
              "pkg_revision = @revision;"
    };
    int sql_rc = 0;
    sqlite3_stmt *stmt = NULL;
    sql_map_t *input = NULL;
    sql_map_t *output = NULL;

    assert (db != NULL);
    assert (pkg != NULL);

    shput (input, "@name",        SQLVAL_S (pkg->program_name));
    shput (input, "@version",     SQLVAL_S (pkg->program_version));
    shput (input, "@revision",    SQLVAL_I (pkg->package_revision));

    sql_rc = dbmap_prepare_v2 (db, SELECT_PACKAGE, -1, input, &stmt);
    if (sql_rc != SQLITE_OK) goto early_exit;

    sql_rc = dbmap_step (stmt, &output);
    if (sql_rc == SQLITE_ROW) 
    {
        (void)package_from_map (p_match, output);
    }

early_exit:
    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);

    shfree (input);
    shfree (output);

    return 0;
}

int
db_package_set_active (sqlite3 *db, package_t *pkg, int active_status)
{
    const char UPDATE_PACKAGE[] = {
        "UPDATE package "
        "SET active = @active "
        "WHERE package_id = @packageid;"
    };

    int sql_rc = 0;
    sql_map_t *input = NULL;

    assert (db != NULL);
    assert (pkg != NULL);
    assert (pkg->package_id > 0);
    assert (active_status == 0 || active_status == 1);

    shput (input, "@packageid", SQLVAL_I (pkg->package_id));
    shput (input, "@active",    SQLVAL_I (active_status));

    sql_rc = dbmap_execute (db, UPDATE_PACKAGE, -1, input);

    shfree (input);

    return EXPECT (sql_rc == SQLITE_DONE);
}



int
db_package_install (sqlite3 *db, package_t *pkg)
{
    const char INSERT_PACKAGE[] = {
        "INSERT INTO package (name, version, license, homepage, pkg_revision, "
                             "pkg_maintainer_name, pkg_maintainer_email, "
                             "automatic, active) "
        "VALUES (@name, @version, @license, @homepage, @revision, @maint_name,"
                "@maint_email, @automatic, True);"
    };
    int rc = 0;
    int sql_rc = 0;
    sql_map_t *input = NULL;
    package_t pkg_match = { 0 };
    int install_status = 0;

    shput (input, "@name",        SQLVAL_S (pkg->program_name));
    shput (input, "@version",     SQLVAL_S (pkg->program_version));
    shput (input, "@license",     SQLVAL_S (pkg->program_license));
    shput (input, "@homepage",    SQLVAL_S (pkg->program_homepage));
    shput (input, "@revision",    SQLVAL_I (pkg->package_revision));
    shput (input, "@maint_name",  SQLVAL_S (pkg->maintainer_name));
    shput (input, "@maint_email", SQLVAL_S (pkg->maintainer_email));
    shput (input, "@automatic",   SQLVAL_I (pkg->package_automatic));

    (void)db_package_select (db, pkg, &pkg_match);
    install_status = (pkg_match.package_id > 0);

    if (install_status && !pkg_match.package_active)
    {
        /* reactivate */
        pkg->package_id = pkg_match.package_id;
        if (db_package_set_active (db, pkg, 1)) goto early_exit;
    }
    else
    {
        if (install_status && pkg_match.package_active)
        {
            /* reinstall */
            if (db_package_uninstall (db, pkg)) goto early_exit;
        }

        /* install */
        sql_rc = dbmap_execute (db, INSERT_PACKAGE, -1, input);
        rc = EXPECT (SQLITE_DONE == sql_rc);
    }

early_exit:
    package_free (&pkg_match);
    shfree (input);

    if (rc)
    {
        fprintf (stderr, "error: dbmap_execute: %d - %s\n",
                sql_rc, sqlite3_errstr (sql_rc));
    }
    return rc;
}

int
db_package_uninstall (sqlite3 *db, package_t *pkg)
{
    const char DEACTIVATE_PACKAGE[] = {
        "UPDATE package "
        "SET active = False "
        "WHERE package_id = @id AND "
              "active = True;"
    };
    int sql_rc = 0;
    sql_map_t *input = NULL;

    shput (input, "@id", SQLVAL_I (pkg->package_id));

    sql_rc = dbmap_execute (db, DEACTIVATE_PACKAGE, -1, input);
    shfree (input);
    input = NULL;

    if (sql_rc != SQLITE_DONE)
    {
        fprintf (stderr, "error: dbmap_execute: %d - %s\n",
                sql_rc, sqlite3_errstr (sql_rc));
        return 1;
    }

    return 0;
}

int
db_package_get (sqlite3 *db, const char *name, package_t *p_ret_pkg)
{
    const char SELECT_PKG_BY_NAME[] = {
        "SELECT * "
        "FROM package "
        "WHERE name   = @name AND "
              "active = True "
        "ORDER BY version DESC;"
    };
    int sql_rc = 0;
    sqlite3_stmt *stmt = NULL;
    package_t pkg_match = { 0 };
    sql_map_t *input = NULL;
    sql_map_t *output = NULL;

    package_init (&pkg_match);

    shput (input, "@name", SQLVAL_S ((char *)name));

    sql_rc = dbmap_prepare_v2 (db, SELECT_PKG_BY_NAME, -1, input, &stmt);
    if (sql_rc != SQLITE_OK) goto early_exit;

    sql_rc = dbmap_step (stmt, &output);
    if (sql_rc == SQLITE_ROW)
    {
        (void)package_from_map (&pkg_match, output);
    }

early_exit:
    sqlite3_reset (stmt);
    sqlite3_finalize (stmt);

    shfree (output);
    shfree (input);

    *p_ret_pkg = pkg_match;
    return EXPECT (sql_rc == SQLITE_ROW);
}

/* vim: fdm=marker
 * end of file */

