/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_sinf.S,v 1.2 1995/04/28 22:44:25 jtc Exp $")

/* A float's domain isn't large enough to require argument reduction. */
ENTRY(sinf)
	flds	4(%esp)
	fsin
	ret
