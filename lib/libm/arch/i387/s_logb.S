/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_logb.S,v 1.3 1995/04/28 22:44:13 jtc Exp $")

ENTRY(logb)
	fldl	4(%esp)
	fxtract
	fstpl	%st
	ret
