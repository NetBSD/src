/*	$NetBSD: __dn_comp.c,v 1.1 1997/04/22 06:55:37 mrg Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(__dn_comp,dn_comp);
#else

#include <sys/types.h>
#Include <resolv.h>

extern int __dn_comp __P((const char *, u_char *, u_char *, u_char *, int));

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
