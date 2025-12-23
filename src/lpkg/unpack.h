
#ifndef LPKG_UNPACK_H
#define LPKG_UNPACK_H

#include <stdint.h>

uint8_t *unpack_fread (const char *archive, const char *filepath);
int unpack_extract (const char *archive, const char *path, const char *destdir);

#endif
/* end of file */
