/*	$NetBSD: __setjmp14.S,v 1.6 2023/10/11 09:12:21 skrll Exp $	*/

#include "SYS.h"
#include "assym.h"

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: __setjmp14.S,v 1.6 2023/10/11 09:12:21 skrll Exp $")
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
	mr	%r6,%r3
	li	%r3,0			# how is ignored
	li	%r4,0			# set = NULL
	addi	%r5,%r6,4*(1+24)	# &sigmask
	_DOSYSCALL(__sigprocmask14)	# assume no error	XXX
	mflr	%r11
	mfcr	%r12
	mr	%r10,%r1
	mr	%r9,%r2
	stmw	%r8,4(%r6)		# save r8-r31
	li	%r3,0
	blr
END(__setjmp14)

ENTRY(__longjmp14)
	lmw	%r8,4(%r3)		# load r8-r31
	mr	%r6,%r4
	mtlr	%r11
	mtcr	%r12
	mr	%r2,%r9
	mr	%r1,%r10
	addi	%r4,%r3,4*(1+24)	# &sigmask
	li	%r3,SIG_SETMASK
	li	%r5,0
	_DOSYSCALL(__sigprocmask14)	# assume no error	XXX
	or.	%r3,%r6,%r6
	bnelr
	li	%r3,1
	blr
END(__longjmp14)
