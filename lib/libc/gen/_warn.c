/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_warn, warn);
#else

#define _warn  warn
#define rcsid   _rcsid
#include "warn.c"

#endif
