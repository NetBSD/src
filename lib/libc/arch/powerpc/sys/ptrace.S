/*	$NetBSD: ptrace.S,v 1.5 2011/01/15 07:31:13 matt Exp $	*/

#include "SYS.h"
#include "assym.h"

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ptrace.S,v 1.5 2011/01/15 07:31:13 matt Exp $")
#endif /* LIBC_SCCS && !lint */

#define	XCALLFRAMELEN		((((2+4)*SZREG) + CALLFRAMELEN - 1) & -CALLFRAMELEN)
#define	XCALLFRAME_DATA		(XCALLFRAMELEN - 1*SZREG)
#define	XCALLFRAME_ADDR		(XCALLFRAMELEN - 2*SZREG)
#define	XCALLFRAME_PID		(XCALLFRAMELEN - 3*SZREG)
#define	XCALLFRAME_REQUEST	(XCALLFRAMELEN - 4*SZREG)

ENTRY(ptrace)
#ifdef _REENTRANT
	mflr	%r0
	stw	%r0,CALLFRAME_LR(%r1)
	stwu	%r1,-XCALLFRAMELEN(%r1)
	stw	%r3,XCALLFRAME_REQUEST(%r1)
	stw	%r4,XCALLFRAME_PID(%r1)
	stw	%r5,XCALLFRAME_ADDR(%r1)
	stw	%r6,XCALLFRAME_DATA(%r1)

	bl	PIC_PLT(_C_LABEL(__errno))
	li	%r7,0
	stw	%r7,0(%r3)

	lwz	%r6,XCALLFRAME_DATA(%r1)
	lwz	%r4,XCALLFRAME_PID(%r1)
	lwz	%r5,XCALLFRAME_ADDR(%r1)
	lwz	%r3,XCALLFRAME_REQUEST(%r1)
	addi	%r1,%r1,XCALLFRAMELEN
	lwz	%r0,CALLFRAME_LR(%r1)
	mtlr	%r0
#else
#ifdef PIC
	mflr	%r0
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	%r7
	lwz	%r7,_C_LABEL(errno)@got(%r7)
	li	%r8,0
	stw	%r8,0(%r7)
	mtlr	0
#else
	lis	%r7,_C_LABEL(errno)@ha
	li	%r8,0
	stw	%r8,_C_LABEL(errno)@l(%r7)
#endif /* PIC */
#endif /* _REENTRANT */
	_DOSYSCALL(ptrace)
	bso	1f
	blr
1:
	b	_C_LABEL(__cerror)
END(ptrace)
