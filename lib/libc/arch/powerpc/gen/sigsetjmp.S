/*	$NetBSD: sigsetjmp.S,v 1.3 1998/07/02 18:40:24 tsubai Exp $	*/

#include <sys/syscall.h>

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	.text
	.asciz "$NetBSD: sigsetjmp.S,v 1.3 1998/07/02 18:40:24 tsubai Exp $"
#endif

ENTRY(sigsetjmp)
	mr	6,3
	or.	7,4,4
	beq	1f
	li	3,1			# SIG_BLOCK
	li	4,0
	li	0,SYS_sigprocmask
	sc				# assume no error	XXX
1:
	mflr	11
	mfcr	12
	mr	10,1
	mr	9,2
	mr	8,3
	stmw	7,0(6)
	li	3,0
	blr

ENTRY(siglongjmp)
	lmw	7,0(3)
	mr	6,4
	mtlr	11
	mtcr	12
	mr	2,9
	mr	1,10
	or.	7,7,7
	beq	1f
	mr	4,8
	li	3,3			# SIG_SETMASK
	li	0,SYS_sigprocmask
	sc				# assume no error	XXX
1:
	or.	3,6,6
	bnelr
	li	3,1
	blr
