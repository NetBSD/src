/*	$NetBSD: strncat.S,v 1.5 1999/11/16 18:17:39 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strncat.S,v 1.5 1999/11/16 18:17:39 matthias Exp $")
#endif

/*
 * char *
 * strncat(char *d, const char *s, size_t count)
 *	append string s to d. append not more then count bytes.
 */
KENTRY(strncat, 12)
	enter	[r4],0
	movd	B_ARG0,tos
	bsr	_C_LABEL(strlen)
#if !defined(MRTD)
	adjspd	-4
#endif
	addd	B_ARG0,r0
	movd	B_ARG1,r1
	movd	r0,r2
	movd	B_ARG2,r0
	movqd	0,r4
	movsb	u
	movb	r4,0(r2)
	movd	B_ARG0,r0
	exit	[r4]
	ret	ARGS
