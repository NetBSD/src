/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_logbf.S,v 1.2 1995/04/28 22:44:14 jtc Exp $")

ENTRY(logbf)
	flds	4(%esp)
	fxtract
	fstpl	%st
	ret
