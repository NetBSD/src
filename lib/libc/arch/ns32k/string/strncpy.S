/*	$NetBSD: strncpy.S,v 1.2 1997/05/08 13:39:00 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strncpy.S,v 1.2 1997/05/08 13:39:00 matthias Exp $")
#endif

/*
 * char *
 * strncpy (char *d, const char *s, size_t len)
 *	copy at most len characters from the  string s to d.
 */

ENTRY(strncpy)
	enter	[r4],0
	movd	B_ARG2,r0
	movd	B_ARG1,r1
	movd	B_ARG0,r2
	movqd	0,r4
	movsb	u
	bfc	0f
	movd	r0,tos
	movd	r2,tos
	bsr	_bzero
	adjspd	-8
0:	movd	B_ARG0,r0
	exit	[r4]
	ret	0
