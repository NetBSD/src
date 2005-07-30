/*	$NetBSD: __res_close.c,v 1.3 2005/07/30 15:21:20 christos Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __res_close.c,v 1.3 2005/07/30 15:21:20 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference) && !defined(__lint__)
__indr_reference(__res_close,res_close)
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

/* XXX THIS IS A MESS!  SEE <resolv.h> XXX */

#undef res_close
void	res_close __P((void));

void
res_close()
{

	__res_close();
}

#endif
