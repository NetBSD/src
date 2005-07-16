/*	$NetBSD: _err.c,v 1.10 2005/07/16 18:01:38 christos Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _err.c,v 1.10 2005/07/16 18:01:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference) && !defined(__lint__)
__indr_reference(_err, err)
#else

#define	__NO_NAMESPACE_H	/* XXX */
#define	_err	err
#define	_verr	verr
#define	rcsid	_rcsid
#include "err.c"

#endif
