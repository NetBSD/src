/*	$NetBSD: __dn_comp.c,v 1.2 1997/04/22 18:53:16 cgd Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(__dn_comp,dn_comp);
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

extern int __dn_comp __P((const char *, u_char *, int, u_char **, u_char **));

#undef dn_comp
int
dn_comp(exp_dn, comp_dn, length, dnptrs, lastdnptr)
	const char *exp_dn;
	u_char *comp_dn, **dnptrs, **lastdnptr;
	int length;
{

	return __dn_comp(exp_dn, comp_dn, length, dnptrs, lastdnptr);
}

#endif
