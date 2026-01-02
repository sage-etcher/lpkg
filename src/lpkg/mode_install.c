
#include "mode.h"

#include "conio.h"
#include "dbio.h"
#include "fileio.h"
#include "lpkg.h"
#include "package.h"
#include "unpack.h"
#include "version.h"

#include <ini.h>
#include <sqlite3.h>

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int pkg_install (char *pkg, int yflag);

static void
install_usage (void)
{
    (void)fprintf (stderr, "Usage: lpkg install [-f] package [package...]\n");
    exit (EXIT_FAILURE);
}

int
lpkg_install_main (int argc, char **argv)
{
    int ch, yflag = 0;

    optind = 1;
    while ((ch = getopt (argc, argv, "+y")) != -1)
    {
        switch (ch)
        {
        case 'y': yflag = 1; break;

        default:
            install_usage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0)
    {
        install_usage ();
    }

    while (argc > 0)
    {
        if (pkg_install (*argv, yflag))
        {
            fprintf (stderr, "error: failed to install package - %s\n", *argv);
            exit (EXIT_FAILURE);
        }

        argc--;
        argv++;
    }

    return 0;
}

static int
handler (void *user, const char *section, const char *name, const char *value)
{
    package_t *p_pkg = (package_t *)user;

    #define MATCH(s, n) strcmp (section, s) == 0 && strcmp (name, n) == 0
    if (MATCH ("program", "name"))
    {
        p_pkg->program_name = strdup (value);
    }
    else if (MATCH ("program", "version"))
    {
        p_pkg->program_version = strdup (value);
    }
    else if (MATCH ("program", "license"))
    {
        p_pkg->program_license = strdup (value);
    }
    else if (MATCH ("program", "homepage"))
    {
        p_pkg->program_homepage = strdup (value);
    }
    else if (MATCH ("package", "revision"))
    {
        p_pkg->package_revision = atoi (value);
    }
    else if (MATCH ("package", "dependencies"))
    {
        p_pkg->package_dependencies = strdup (value);
    }
    else if (MATCH ("maintainer", "name"))
    {
        p_pkg->maintainer_name = strdup (value);
    }
    else if (MATCH ("maintainer", "email"))
    {
        p_pkg->maintainer_email = strdup (value);
    }
    else
    {
        fprintf (stderr, "warning: unknown package.ini entry - %s.%s: %s\n",
                section, name, value);
        return 0;
    }

    return 1;
}


static int
pkg_install (char *pkg_name, int yflag)
{
    sqlite3 *db = NULL;
    char *package_ini = NULL;
    package_t new_pkg = { 0 };
    package_t old_pkg = { 0 };
    int ver_cmp = 0;
    int rev_cmp = 0;
    int c = 0;

    /* check if pkg_name exists */
    if (!file_exists (pkg_name))
    {
        fprintf (stderr, "error: no such package - %s\n", pkg_name);
        return -1;
    }

    /* read+parse the package.ini from package */
    package_ini = (char *)unpack_fread (pkg_name, "package.ini");
    assert (package_ini != NULL);

    package_init (&new_pkg);
    if (ini_parse_string (package_ini, handler, &new_pkg) < 0)
    {
        fprintf (stderr, "error: cannot parse package\n");
        free (package_ini);
        return 1;
    }

    free (package_ini);
    package_ini = NULL;

    /* open database */
    db = db_open (g_dbfile);
    if (db == NULL)
    {
        fprintf (stderr, "error: failed to open database\n");
        package_free (&new_pkg);
        return -1;
    }

    /* check if database has a package by the same name */
    package_init (&old_pkg);
    if (!db_package_get (db, new_pkg.program_name, &old_pkg))
    {
        /* compare versions+revision of old package vs new */
        ver_cmp = version_cmp (old_pkg.program_version, new_pkg.program_version);
        rev_cmp = old_pkg.package_revision - new_pkg.package_revision;

        if ((ver_cmp < 0) || (ver_cmp == 0 && rev_cmp < 0))
        {
            printf ("provided package is newer, update?");
        }
        else if ((ver_cmp > 0) || (ver_cmp == 0 && rev_cmp > 0))
        {
            printf ("provided package is older, downgrade?");
        }
        else if ((ver_cmp == 0) || (rev_cmp == 0))
        {
            printf ("package already installed, reinstall?\n");
        }
        else 
        {
            fprintf (stderr, "error: failed to compare package versioning\n");
            package_free (&old_pkg);
            package_free (&new_pkg);
            return -1;
        }

        if (!yflag)
        {
            c = prompt ("would you like to continue? [Yn] ");
            if (c == 'n')
            {
                printf ("cancelling package install\n");
                package_free (&old_pkg);
                package_free (&new_pkg);
                return -1;
            }
        }

        if (db_package_uninstall (db, &old_pkg))
        {
            fprintf (stderr, "error: failure to remove package\n");
            package_free (&old_pkg);
            package_free (&new_pkg);
            return -1;
        }
    }

    if (db_package_install (db, &new_pkg))
    {
        fprintf (stderr, "error: failure to install package\n");
        package_free (&old_pkg);
        package_free (&new_pkg);
        return -1;
    }

    package_free (&old_pkg);
    package_free (&new_pkg);
    return 0;
}


/* end of file */
