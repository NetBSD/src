/*	$NetBSD: strcat.S,v 1.2 1997/05/08 13:38:55 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strcat.S,v 1.2 1997/05/08 13:38:55 matthias Exp $")
#endif

/*
 * char *
 * strcat (char *d, const char *s)
 *	append string s to d.
 */

ENTRY(strcat)
	enter	[],0
	movd	B_ARG0,tos
	bsr	_strlen
	addd	B_ARG0,r0
	movd	B_ARG1,0(sp)
	movd	r0,tos
	bsr	_strcpy
	adjspd	-8
	movd	B_ARG0,r0
	exit	[]
	ret	0
