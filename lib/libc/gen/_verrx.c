/*	$NetBSD: _verrx.c,v 1.8 2005/06/12 05:34:34 lukem Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _verrx.c,v 1.8 2005/06/12 05:34:34 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef __indr_reference
__indr_reference(_verrx, verrx)
#else

#define	__NO_NAMESPACE_H	/* XXX */
#define _verrx	verrx
#define rcsid   _rcsid
#include "verrx.c"

#endif
