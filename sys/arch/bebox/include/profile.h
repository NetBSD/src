/*	$NetBSD: profile.h,v 1.1 1997/10/14 06:48:35 sakamoto Exp $	*/

#define	_MCOUNT_DECL	void mcount

/* XXX implement XXX */

#define	MCOUNT

#ifdef _KERNEL
#define	MCOUNT_ENTER
#define	MCONT_EXIT
#endif
