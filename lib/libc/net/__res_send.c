/*	$NetBSD: __res_send.c,v 1.1 1997/04/22 06:55:39 mrg Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(__res_send,res_send);
#else

#include <sys/types.h>
#include <resolv.h>

extern int __res_send __P((const u_char *, int, u_char *, int));

#undef res_send
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
