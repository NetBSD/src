/*	$NetBSD: strcat.S,v 1.2.20.1 1999/12/27 18:35:59 wrstuden Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strcat.S,v 1.2.20.1 1999/12/27 18:35:59 wrstuden Exp $")
#endif

/*
 * char *
 * strcat (char *d, const char *s)
 *	append string s to d.
 */

KENTRY(strcat, 8)
	enter	[],0
	movd	B_ARG0,tos
	bsr	_strlen
	addd	B_ARG0,r0
	movd	B_ARG1,0(sp)
	movd	r0,tos
	bsr	_strcpy
#if !defined(MRTD)
	adjspd	-8
#endif
	movd	B_ARG0,r0
	exit	[]
	ret	ARGS
