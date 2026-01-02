
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

int package_from_map (package_t *self, sql_map_t *map);

void package_log (package_t *self);

#endif
/* end of file */
