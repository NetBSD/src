/*	$NetBSD: profile.h,v 1.1 1998/05/15 10:15:55 tsubai Exp $	*/

#define	_MCOUNT_DECL	void mcount

/* XXX implement XXX */

#define	MCOUNT

#ifdef _KERNEL
#define	MCOUNT_ENTER
#define	MCONT_EXIT
#endif
