/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_significand.S,v 1.3 1995/04/28 22:44:20 jtc Exp $")

ENTRY(significand)
	fldl	4(%esp)
	fxtract
	fstpl	%st(1)
	ret
