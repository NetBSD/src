/*	$NetBSD: __res_send.c,v 1.3 1997/07/17 21:27:34 thorpej Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(__res_send,res_send);
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
