/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_floorf.S,v 1.4 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(floorf)
#ifdef __i386__
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp

	fstcw	-12(%ebp)		/* store fpu control word */
	movw	-12(%ebp),%dx
	orw	$0x0400,%dx		/* round towards -oo */
	andw	$0xf7ff,%dx
	movw	%dx,-16(%ebp)
	fldcw	-16(%ebp)		/* load modfied control word */

	flds	8(%ebp);		/* round */
	frndint

	fldcw	-12(%ebp)		/* restore original control word */

	leave
#else
	movss	%xmm0, -4(%rsp)
	fstcw	-8(%rsp)
	movw	-8(%rsp),%dx
	orw	$0x0400,%dx
	andw	$0xf7ff,%dx  
	movw	%dx,-12(%rsp)
	fldcw	-12(%rsp)
	fldl	-4(%rsp)
	frndint
	fldcw	-8(%rsp)
	fstpl	-4(%rsp)
	movss	-4(%rsp),%xmm0
#endif
	ret
