/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_ilogb.S,v 1.3 1995/04/28 22:44:08 jtc Exp $")

ENTRY(ilogb)
	pushl	%esp
	movl	%esp,%ebp
	subl	$4,%esp

	fldl	8(%ebp)
	fxtract
	fstpl	%st

	fistpl	-4(%ebp)
	movl	-4(%ebp),%eax

	leave
	ret
