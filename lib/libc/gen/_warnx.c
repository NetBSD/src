/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_warnx, warnx);
#else

#define _warnx  warnx
#define rcsid   _rcsid
#include "warnx.c"

#endif
