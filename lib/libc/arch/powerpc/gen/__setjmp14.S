/*	$NetBSD: __setjmp14.S,v 1.1 1998/10/03 12:30:38 tsubai Exp $	*/

#include <sys/syscall.h>

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	.text
	.asciz "$NetBSD: __setjmp14.S,v 1.1 1998/10/03 12:30:38 tsubai Exp $"
#endif

/*
 * C library -- _setjmp, _longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v?v:1)" from the last call to
 *	setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is restored.
 */

ENTRY(__setjmp14)
	mr	6,3
	li	3,1			# SIG_BLOCK
	li	4,0
	addi	5,6,100			# &sigmask
	li	0,SYS___sigprocmask14
	sc				# assume no error	XXX
	mflr	11
	mfcr	12
	mr	10,1
	mr	9,2
	stmw	8,4(6)			# save r8-r31
	li	3,0
	blr

ENTRY(__longjmp14)
	lmw	8,4(3)			# load r8-r31
	mr	6,4
	mtlr	11
	mtcr	12
	mr	2,9
	mr	1,10
	addi	4,3,100			# &sigmask
	li	3,3			# SIG_SETMASK
	li	5,0
	li	0,SYS___sigprocmask14
	sc				# assume no error	XXX
	or.	3,6,6
	bnelr
	li	3,1
	blr
