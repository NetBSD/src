/*	$NetBSD: strcat.S,v 1.4 1998/10/01 22:46:37 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strcat.S,v 1.4 1998/10/01 22:46:37 matthias Exp $")
#endif

/*
 * char *
 * strcat (char *d, const char *s)
 *	append string s to d.
 */

KENTRY(strcat, 8)
	enter	[],0
	movd	B_ARG0,tos
	bsr	_C_LABEL(strlen)
	addd	B_ARG0,r0
	movd	B_ARG1,0(sp)
	movd	r0,tos
	bsr	_C_LABEL(strcpy)
	adjspd	-8
	movd	B_ARG0,r0
	exit	[]
	ret	ARGS
