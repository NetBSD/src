/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_sin.S,v 1.4 1995/04/28 22:44:23 jtc Exp $")

ENTRY(sin)
	fldl	4(%esp)
	fsin
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
	fsin
	ret
