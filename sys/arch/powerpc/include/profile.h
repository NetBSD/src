/*	$NetBSD: profile.h,v 1.3 1997/04/16 23:00:53 thorpej Exp $	*/

#define	_MCOUNT_DECL	void mcount

/* XXX implement XXX */

#define	MCOUNT

#ifdef _KERNEL
#define	MCOUNT_ENTER
#define	MCONT_EXIT
#endif
