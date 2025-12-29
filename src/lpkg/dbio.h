
#ifndef LPKG_DBIO_H
#define LPKG_DBIO_H

#include "package.h"

#include <sqlite3.h>

sqlite3 *db_open (const char *filepath);
int db_close (sqlite3 *db);

int db_init_tables (sqlite3 *db);

int db_bind_vvv_int (sqlite3_stmt *stmt, const char *name, int value);
int db_bind_vvv_text (sqlite3_stmt *stmt, const char *name, const char *text, 
int text_length, void (*free_cb)(void *));

int db_transaction (sqlite3 *db, const char *fmt, ...);

int db_package_install (sqlite3 *db, package_t *p_pkg);
int db_package_uninstall (sqlite3 *db, package_t *p_pkg);
int db_get_package (sqlite3 *db, const char *name, package_t *p_ret_pkg);


#endif
/* end of file */
