
#include "package.h"

#include "dbmap.h"

#include <stb_ds.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


void
package_init (package_t *self)
{
    *self = (package_t){
        .package_id = 0,

        .program_name = NULL,
        .program_version = NULL,
        .program_license = NULL,
        .program_homepage = NULL,

        .package_revision = 0,
        .package_automatic = 0,
        .package_dependencies = NULL,
        .package_active = 1,

        .maintainer_name = NULL,
        .maintainer_email = NULL,
    };
}

void
package_free (package_t *self)
{
    if (self == NULL) return;

    free (self->program_name);
    free (self->program_version);
    free (self->program_license);
    free (self->program_homepage);
    free (self->package_dependencies);
    free (self->maintainer_name);
    free (self->maintainer_email);
}

int
package_from_map (package_t *self, sql_map_t *map)
{
    struct { const char *key; void *dest; } entries[] = {
        { "package_id",           &self->package_id },
        { "name",                 &self->program_name },
        { "version",              &self->program_version },
        { "license",              &self->program_license },
        { "homepage",             &self->program_homepage },
        { "automatic",            &self->package_automatic },
        { "pkg_revision",         &self->package_revision },
        { "active",               &self->package_active },
        { "pkg_maintainer_name",  &self->maintainer_name },
        { "pkg_maintainer_email", &self->maintainer_email },
    };
    size_t entry_count = sizeof (entries) / sizeof (*entries);
    size_t i = 0;
    int index = -1;

    for (i = 0; i < entry_count; i++)
    {
        index = shgeti (map, entries[i].key);
        if (index < 0) 
        {
            fprintf (stderr, "warning: map does not have any \"%s\" key\n",
                    entries[i].key);
            continue;
        }

        switch (map[index].value.sql_type)
        {
        case SQLITE_INTEGER: 
            *(int *)(entries[i].dest) = map[index].value.m.i;
            break;

        case SQLITE_TEXT: 
            *(char **)(entries[i].dest) = strdup (map[index].value.m.s);
            break;

        default:
            fprintf (stderr, "error: package_from_map unhandled type\n");
            return 1;
        }
    }

    return 0;
}

void
package_log (package_t *self)
{
    printf ("%d:%s:%s:%s:%s:%d:%d:%s:%s:%s\n",
            self->package_id, self->program_name, self->program_version,
            self->program_license, self->program_homepage, 
            self->package_revision, self->package_automatic, 
            self->package_dependencies, 
            self->maintainer_name, self->maintainer_email);
}


/* end of file */
