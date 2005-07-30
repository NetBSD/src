/*	$NetBSD: __res_send.c,v 1.3 2005/07/30 15:21:20 christos Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __res_send.c,v 1.3 2005/07/30 15:21:20 christos Exp $");
#endif

#if defined(__indr_reference) && !defined(__lint__)
__indr_reference(__res_send,res_send)
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

/* XXX THIS IS A MESS!  SEE <resolv.h> XXX */

#undef res_send
int	res_send __P((const u_char *, int, u_char *, int));

int
res_send(buf, buflen, ans, anssiz)
	const u_char *buf;
	int buflen;
	u_char *ans;
	int anssiz;
{

	return __res_send(buf, buflen, ans, anssiz);
}

#endif
