/*	$NetBSD: __res_close.c,v 1.3 1997/07/17 21:27:34 thorpej Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(__res_close,_res_close);
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

/* XXX THIS IS A MESS!  SEE <resolv.h> XXX */

#undef _res_close
void	_res_close __P((void));

void
_res_close()
{

	__res_close();
}

#endif
