/*	$NetBSD: __res_close.c,v 1.1 2004/05/20 18:00:55 christos Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

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
