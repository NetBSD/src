/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_log1p.S,v 1.6 1995/04/28 22:44:10 jtc Exp $")

/*
 * Since the fyl2xp1 instruction has such a limited range:
 *	-(1 - (sqrt(2) / 2)) <= x <= sqrt(2) - 1
 * it's not worth trying to use it.  
 */

ENTRY(log1p)
	fldln2
	fldl 4(%esp)
	fld1
	faddp
	fyl2x
	ret
