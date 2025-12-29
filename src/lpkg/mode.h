
#ifndef LPKG_MODE_H
#define LPKG_MODE_H

/* int lpkg_autoremove_main (int argc, char **argv); */
int lpkg_dbquery_main (int argc, char **argv);
/* int lpkg_info_main (int argc, char **argv); */
int lpkg_init_main (int argc, char **argv);
int lpkg_install_main (int argc, char **argv);
/* int lpkg_list_main (int argc, char **argv); */
int lpkg_remove_main (int argc, char **argv);
/* int lpkg_update_main (int argc, char **argv); */

int lpkg_unimplemented_main (int argc, char **argv);
#define lpkg_autoremove_main lpkg_unimplemented_main
#define lpkg_info_main       lpkg_unimplemented_main
#define lpkg_list_main       lpkg_unimplemented_main
#define lpkg_update_main     lpkg_unimplemented_main

#endif
/* end of file */
