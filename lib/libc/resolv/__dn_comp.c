/*	$NetBSD: __dn_comp.c,v 1.2 2005/06/12 05:21:27 lukem Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __dn_comp.c,v 1.2 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef __indr_reference
__indr_reference(__dn_comp,dn_comp)
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

/* XXX THIS IS A MESS!  SEE <resolv.h> XXX */

#undef dn_comp
int	dn_comp __P((const char *, u_char *, int, u_char **, u_char **));

int
dn_comp(exp_dn, comp_dn, length, dnptrs, lastdnptr)
	const char *exp_dn;
	u_char *comp_dn, **dnptrs, **lastdnptr;
	int length;
{

	return __dn_comp(exp_dn, comp_dn, length, dnptrs, lastdnptr);
}

#endif
