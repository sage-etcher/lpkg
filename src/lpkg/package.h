
#ifndef LPKG_PACKAGE
#define LPKG_PACKAGE

typedef struct {
    char *program_name;
    char *program_version;
    char *program_license;
    char *program_homepage;

    int package_revision;
    char *package_dependencies;

    char *maintainer_name;
    char *maintainer_email;
} package_t;

void package_init (package_t *self);
void package_free (package_t *self);

#endif
/* end of file */
