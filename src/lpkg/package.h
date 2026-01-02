
#ifndef LPKG_PACKAGE
#define LPKG_PACKAGE

#include "dbmap.h"

typedef struct {
    int package_id;

    char *program_name;
    char *program_version;
    char *program_license;
    char *program_homepage;

    int package_revision;
    int package_automatic;
    char *package_dependencies;
    int package_active;

    char *maintainer_name;
    char *maintainer_email;
} package_t;

void package_init (package_t *self);
void package_free (package_t *self);

package_t package_from_map (sql_map_t *map);
sql_map_t *package_to_map (package_t *pkg);

void package_log (package_t *self);

#endif
/* end of file */
