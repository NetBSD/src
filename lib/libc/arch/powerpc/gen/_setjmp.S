/*	$NetBSD: _setjmp.S,v 1.2 2002/07/30 06:07:57 matt Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	.text
	.asciz "$NetBSD: _setjmp.S,v 1.2 2002/07/30 06:07:57 matt Exp $"
#endif

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v?v:1)" from the last call to
 *	_setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is NOT restored.
 */

ENTRY(_setjmp)
	mflr	%r11			/* save return address */
	mfcr	%r12			/* save condition register */
	mr	%r10,%r1		/* save stack pointer */
	mr	%r9,%r2			/* save GPR2 (not needed) */
	stmw	%r9,8(%r3)		/* save r9..r31 */
	li	%r3,0			/* indicate success */
	blr				/* return */

ENTRY(_longjmp)
	lmw	%r9,8(%r3)		/* save r9..r31 */
	mtlr	%r11			/* restore LR */
	mtcr	%r12			/* restore CR */
	mr	%r2,%r9			/* restore GPR2 (not needed) */
	mr	%r1,%r10		/* restore stack */
	or.	%r3,%r4,%r4		/* get return value */
	bnelr				/* return if not 0 */
	li	%r3,1			/* what's the point? */
	blr				/* return */
