/*	$NetBSD: setjmp.S,v 1.2 1998/05/25 15:28:02 ws Exp $	*/

#include <sys/syscall.h>

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	.text
	.asciz "$NetBSD: setjmp.S,v 1.2 1998/05/25 15:28:02 ws Exp $"
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

ENTRY(setjmp)
	mr	6,3
	li	3,1			# SIG_BLOCK
	li	4,0
	li	0,SYS_sigprocmask
	sc				# assume no error	XXX
	mflr	11
	mfcr	12
	mr	10,1
	mr	9,2
	mr	8,3
	stmw	8,4(6)
	li	3,0
	blr

ENTRY(longjmp)
	lmw	8,4(3)
	mr	6,4
	mtlr	11
	mtcr	12
	mr	2,9
	mr	1,10
	mr	4,8
	li	3,3			# SIG_SETMASK
	li	0,SYS_sigprocmask
	sc				# assume no error	XXX
	or.	3,6,6
	bnelr
	li	3,1
	blr
