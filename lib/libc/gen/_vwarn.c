/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_vwarn, vwarn);
#else

#define	__NO_NAMESPACE_H	/* XXX */
#define _vwarn	vwarn
#define rcsid   _rcsid
#include "vwarn.c"

#endif
