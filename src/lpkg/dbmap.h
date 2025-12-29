
#ifndef LPKG_DBMAP_H
#define LPKG_DBMAP_H

#include <sqlite3.h>
#include <stb_ds.h>

typedef struct {
    int sql_type;
    union {
        char *s;
        int i;
    } m;
} sql_value_t;

typedef struct {
    const char *key;
    sql_value_t value;
} sql_map_t;

#define SQLVAL_S(value) \
    ((sql_value_t){ .sql_type = SQLITE_TEXT, .m = { .s = (value) } })

#define SQLVAL_I(value) \
    ((sql_value_t){ .sql_type = SQLITE_INTEGER, .m = { .i = (value) } })

int dbmap_bind (sqlite3_stmt *stmt, sql_map_t *bind_args);
int dbmap_step (sqlite3_stmt *stmt, sql_map_t **p_ret_map);

int dbmap_execute (sqlite3 *db, const char *sql_text, int n, sql_map_t *input);

#endif
/* end of file */
