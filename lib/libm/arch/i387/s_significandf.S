/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_significandf.S,v 1.2 1995/04/28 22:44:22 jtc Exp $")

ENTRY(significandf)
	flds	4(%esp)
	fxtract
	fstpl	%st(1)
	ret
