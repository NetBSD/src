/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_cos.S,v 1.4 1995/04/28 22:43:57 jtc Exp $")

ENTRY(cos)
	fldl	4(%esp)
	fcos
	fnstsw	%ax
	andw	$0x400,%ax
	jnz	1f
	ret	
1:	fldpi
	fadd	%st(0)
	fxch	%st(1)
2:	fprem1
	fnstsw	%ax
	andw	$0x400,%ax
	jnz	2b
	fstp	%st(1)
	fcos
	ret
