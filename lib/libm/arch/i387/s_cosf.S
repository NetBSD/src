/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_cosf.S,v 1.2 1995/04/28 22:43:58 jtc Exp $")

/* A float's domain isn't large enough to require argument reduction. */
ENTRY(cosf)
	flds	4(%esp)
	fcos
	ret	
