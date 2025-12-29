
#include "mode.h"

#include "dbio.h"
#include "lpkg.h"
#include "package.h"

#include <assert.h>
#include <stdio.h>

int
lpkg_unimplemented_main (int argc, char **argv)
{
    int rc = 0;
    sqlite3 *db = NULL;
    package_t read_pkg = { 0 };
    package_t pkg = { 0 };


    package_init (&pkg);

    pkg = (package_t){
        .program_name = "test",
        .program_version = "0.0.1",
        .program_license = "MIT",
        .program_homepage = "https://example.com",

        .package_revision = 1,
        .package_automatic = 1,
        .package_dependencies = "",

        .maintainer_name = "sage-etcher",
        .maintainer_email = "sage.signup@email.com",
    };

    db = db_open (g_dbfile);
    assert (db != NULL);

    db_package_install (db, &pkg);
    db_get_package (db, "test", &read_pkg);
    package_log (&read_pkg);

    db_package_uninstall (db, &read_pkg);

    db_get_package (db, "test", &read_pkg);
    package_log (&read_pkg);

    db_close (db);
    fprintf (stderr, "error: unimplemented mode\n");
    return 0;
}


/* end of file */
