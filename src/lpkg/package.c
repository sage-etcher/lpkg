
#include "package.h"

#include <stddef.h>
#include <stdlib.h>


void
package_init (package_t *self)
{
    *self = (package_t){
        .program_name = NULL,
        .program_version = NULL,
        .program_license = NULL,
        .program_homepage = NULL,
        .package_revision = 0,
        .package_dependencies = NULL,
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


/* end of file */
