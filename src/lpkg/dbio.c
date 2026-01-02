
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
static int get_pkg_n_cancel (void *user, sql_map_t *output);


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

/* database helpers */
static int
get_pkg_n_cancel (void *user, sql_map_t *output)
{
    package_t *pkg = user;
    *pkg = package_from_map (output);
    return 1;
}

int
db_exec_pkg (sqlite3 *db, const char *sql, sql_map_t *input, package_t *p_pkg)
{
    int sql_rc = 0;

    assert (db != NULL);
    assert (sql != NULL);
    assert (p_pkg != NULL);

    sql_rc = dbmap_execute_v2 (db, sql, input, get_pkg_n_cancel, p_pkg);
    if (sql_rc != SQLITE_ROW)
    {
        fprintf (stderr, "error: failed to execute SQL - %s\n",
                sql);
        return 1;
    }

    return 0;
}


/* shortcuts */
int
db_package_select_3 (sqlite3 *db, sql_map_t *input, package_t *p_pkg)
{
    const char SELECT_PACKAGE[] = {
        "SELECT * "
        "FROM package "
        "WHERE name         = @program_name     AND "
              "version      = @program_version  AND "
              "pkg_revision = @package_revision;"
    };
    assert (-1 != shgeti (input, "@program_name"));
    assert (-1 != shgeti (input, "@program_version"));
    assert (-1 != shgeti (input, "@package_revision"));

    return db_exec_pkg (db, SELECT_PACKAGE, input, p_pkg);
}

int
db_package_select_2 (sqlite3 *db, sql_map_t *input, package_t *p_pkg)
{
    const char SELECT_PACKAGE[] = {
        "SELECT * "
        "FROM package "
        "WHERE name         = @program_name     AND "
              "version      = @program_version;"
    };
    assert (-1 != shgeti (input, "@program_name"));
    assert (-1 != shgeti (input, "@program_version"));

    return db_exec_pkg (db, SELECT_PACKAGE, input, p_pkg);
}

int
db_package_select_1 (sqlite3 *db, sql_map_t *input, package_t *p_pkg)
{
    const char SELECT_PKG_BY_NAME[] = {
        "SELECT * "
        "FROM package "
        "WHERE name = @program_name "
        "ORDER BY version DESC;"
    };
    assert (-1 != shgeti (input, "@program_name"));

    return db_exec_pkg (db, SELECT_PKG_BY_NAME, input, p_pkg);
}

int
db_package_select (sqlite3 *db, sql_map_t *input, package_t *p_pkg)
{
    if (-1 == shgeti (input, "@program_name")) return 1;

    if (-1 == shgeti (input, "@program_version")) 
    {
        return db_package_select_1 (db, input, p_pkg);
    }

    if (-1 == shgeti (input, "@program_revision")) 
    {
        return db_package_select_2 (db, input, p_pkg);
    }

    return db_package_select_3 (db, input, p_pkg);
}

int
db_package_set_active (sqlite3 *db, sql_map_t *input)
{
    const char UPDATE_PACKAGE[] = {
        "UPDATE package "
        "SET active = @package_active "
        "WHERE package_id = @package_id;"
    };
    assert (-1 != shgeti (input, "@package_active"));
    assert (-1 != shgeti (input, "@package_id"));

    return db_exec_pkg (db, UPDATE_PACKAGE, input, NULL);
}

/* more complex database controls */
int
db_package_uninstall (sqlite3 *db, sql_map_t *input)
{
    int rc = 0;
    package_t match_pkg = { 0 };
    sql_map_t *match_map = NULL;

    package_init (&match_pkg);

    /* search a matching package */
    rc = db_package_select (db, input, &match_pkg);
    if (rc)
    {
        fprintf (stderr, "error: no package found\n");
        goto early_exit;
    }

    /* deactiveate the package */
    match_pkg.package_active = 0;
    match_map = package_to_map (&match_pkg);

    rc = db_package_set_active (db, match_map);
    if (rc)
    {
        fprintf (stderr, "error: failed to deactivate the package\n");
        goto early_exit;
    }

early_exit:
    shfree (match_map);
    package_free (&match_pkg);
    return rc;
}


int
db_package_install (sqlite3 *db, sql_map_t *input)
{
    const char INSERT_PACKAGE[] = {
        "INSERT INTO package (name, version, license, homepage, pkg_revision, "
                             "pkg_maintainer_name, pkg_maintainer_email, "
                             "automatic, active) "
        "VALUES (@program_name, @program_version, @program_license, "
                "@program_homepage, @package_revision, @maintainers_name,"
                "@maintainers_email, @package_automatic, True);"
    };
    int rc = 0;
    int sql_rc = 0;
    package_t match_pkg = { 0 };

    int package_installed = 0;

    package_init (&match_pkg);

    package_installed = db_package_select (db, input, &match_pkg);

    if (!package_installed)
    {
        /* fresh install */
        return db_exec_pkg (db, INSERT_PACKAGE, input, NULL, NULL);
    }
    else 
    {


        if (match_pkg.package_active)
        {
            /* reinstall? */
            rc = db_package_set_active (db, match_map);
            if (rc) return rc;

            return db_exec_pkg (db, INSERT_PACKAGE, match_map, NULL, NULL);
        }
        else
        {
            /* reactivate */
        }
    }




early_exit:
    package_free (&match_pkg);
    return rc;
    









    (void)db_package_select (db, input, &pkg);
    install_status = (pkg.package_id > 0);

    if (install_status && !pkg.package_active)
    {
        /* reactivate */
        pkg->package_id = pkg.package_id;
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

    if (rc)
    {
        fprintf (stderr, "error: dbmap_execute: %d - %s\n",
                sql_rc, sqlite3_errstr (sql_rc));
    }
    return rc;
}


/* vim: fdm=marker
 * end of file */

