/*
 * Just compatibility function prototypes.
 * Public domain.
 */

#ifndef COMPAT_H
#define COMPAT_H

#include <string.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#endif
