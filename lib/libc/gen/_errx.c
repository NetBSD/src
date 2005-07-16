/*	$NetBSD: _errx.c,v 1.10 2005/07/16 18:01:38 christos Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _errx.c,v 1.10 2005/07/16 18:01:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference) && !defined(__lint__)
__indr_reference(_errx, errx)
#else

#define	__NO_NAMESPACE_H	/* XXX */
#define _errx    errx
#define _verrx   verrx
#define rcsid   _rcsid
#include "errx.c"

#endif
