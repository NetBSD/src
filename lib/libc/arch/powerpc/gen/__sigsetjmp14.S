/*	$NetBSD: __sigsetjmp14.S,v 1.1 1998/10/03 12:30:38 tsubai Exp $	*/

#include <sys/syscall.h>

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	.text
	.asciz "$NetBSD: __sigsetjmp14.S,v 1.1 1998/10/03 12:30:38 tsubai Exp $"
#endif

ENTRY(__sigsetjmp14)
	mr	6,3
	or.	7,4,4
	beq	1f
	li	3,1			# SIG_BLOCK
	li	4,0
	addi	5,6,100			# &sigmask
	li	0,SYS___sigprocmask14
	sc				# assume no error	XXX
1:
	mflr	11
	mfcr	12
	mr	10,1
	mr	9,2
	stmw	7,0(6)			# save r7-r31
	li	3,0
	blr

ENTRY(__siglongjmp14)
	lmw	7,0(3)			# load r7-r31
	mr	6,4
	mtlr	11
	mtcr	12
	mr	2,9
	mr	1,10
	or.	7,7,7
	beq	1f
	addi	4,3,100			# &sigmask
	li	3,3			# SIG_SETMASK
	li	5,0
	li	0,SYS___sigprocmask14
	sc				# assume no error	XXX
1:
	or.	3,6,6
	bnelr
	li	3,1
	blr
