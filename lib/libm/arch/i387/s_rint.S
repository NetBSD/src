/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_rint.S,v 1.3 1995/04/28 22:44:15 jtc Exp $")

ENTRY(rint)
	fldl	4(%esp)
	frndint
	ret
