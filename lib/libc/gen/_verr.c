/*	$NetBSD: _verr.c,v 1.7 1998/01/09 03:15:27 perry Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_verr, verr)
#else

#define	__NO_NAMESPACE_H	/* XXX */
#define _verr	verr
#define rcsid	_rcsid
#include "verr.c"

#endif
