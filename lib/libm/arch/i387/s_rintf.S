/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_rintf.S,v 1.2 1995/04/28 22:44:16 jtc Exp $")

ENTRY(rintf)
	flds	4(%esp)
	frndint
	ret
