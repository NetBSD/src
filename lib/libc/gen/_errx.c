/*	$NetBSD: _errx.c,v 1.8 1998/01/09 03:15:24 perry Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_errx, errx)
#else

#define	__NO_NAMESPACE_H	/* XXX */
#define _errx    errx
#define _verrx   verrx
#define rcsid   _rcsid
#include "errx.c"

#endif
