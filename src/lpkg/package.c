
#include "package.h"

#include "dbmap.h"

#include <stb_ds.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TO_MAP(elem, def, t_fn)                                         \
    do {                                                                \
        if (pkg->elem != (def)) shput (map, #elem, t_fn (pkg->elem));   \
    } while (0)

#define FROM_MAP(elem, def, value)                                      \
    do {                                                                \
        index = shgeti (map, #elem);                                    \
        if (index == -1)                                                \
        {                                                               \
            pkg.elem = def;                                             \
        }                                                               \
        else                                                            \
        {                                                               \
            pkg.elem = value;                                           \
        }                                                               \
    } while (0)


void
package_init (package_t *self)
{
    *self = (package_t){
        .package_id = 0,

        .program_name     = NULL,
        .program_version  = NULL,
        .program_license  = NULL,
        .program_homepage = NULL,

        .package_revision  = -1,
        .package_automatic = -1,
        .package_active    = -1,
        .package_dependencies = NULL,

        .maintainer_name  = NULL,
        .maintainer_email = NULL,
    };
}

sql_map_t *
package_to_map (package_t *pkg)
{
    sql_map_t *map = NULL;

    TO_MAP (package_id, 0, SQLVAL_I);

    TO_MAP (program_name,     NULL, SQLVAL_S);
    TO_MAP (program_version,  NULL, SQLVAL_S);
    TO_MAP (program_license,  NULL, SQLVAL_S);
    TO_MAP (program_homepage, NULL, SQLVAL_S);

    TO_MAP (package_revision,  -1, SQLVAL_I);
    TO_MAP (package_automatic, -1, SQLVAL_I);
    TO_MAP (package_active,    -1, SQLVAL_I);
    TO_MAP (package_dependencies, NULL, SQLVAL_S);

    TO_MAP (maintainer_name,  NULL, SQLVAL_S);
    TO_MAP (maintainer_email, NULL, SQLVAL_S);

    return map;
}

package_t
package_from_map (sql_map_t *map)
{
    package_t pkg;
    int index = -1;

    FROM_MAP (package_id, 0, map[index].value.m.i);

    FROM_MAP (program_name,     NULL, strdup (map[index].value.m.s));
    FROM_MAP (program_version,  NULL, strdup (map[index].value.m.s));
    FROM_MAP (program_license,  NULL, strdup (map[index].value.m.s));
    FROM_MAP (program_homepage, NULL, strdup (map[index].value.m.s));

    FROM_MAP (package_revision,  -1, map[index].value.m.i);
    FROM_MAP (package_automatic, -1, map[index].value.m.i);
    FROM_MAP (package_active,    -1, map[index].value.m.i);
    FROM_MAP (package_dependencies, NULL, strdup (map[index].value.m.s));

    FROM_MAP (maintainer_name,  NULL, strdup (map[index].value.m.s));
    FROM_MAP (maintainer_email, NULL, strdup (map[index].value.m.s));

    return pkg;
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
