/*	$NetBSD: __res_close.c,v 1.2 2005/06/12 05:21:27 lukem Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __res_close.c,v 1.2 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef __indr_reference
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
