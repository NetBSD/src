/*	$NetBSD: reenter_syscall.s,v 1.2 1999/11/10 23:17:21 thorpej Exp $	*/

/*
 * Written by ITOH Yasufumi.
 * Public domain.
 */

#include <m68k/asm.h>

/*
 * void reenter_syscall(struct frame *fp, int stkadj)
 *					__attribute__((__noreturn__));
 *
 * Move stack frame by stkadj bytes and re-enter syscall().
 *
 * XXX This is a kludge.
 */

ENTRY_NOPROFILE(reenter_syscall)
	addql	#4,%sp			| pop PC
	movel	%sp@+,%a0		| current frame addr
	movel	%sp@,%d1		| stkadj

| The m68k frame (struct trapframe) format:
|	16:l	d0-d7/a0-a6/usp
|	1:w	(pad)
|	1:w	stkadj
|	1:w	sr
|	1:l	pc
|	1:w	format/vector

	moveal	%a0,%a1
	subal	%d1,%a1			| new frame address
	moveal	%a1,%sp			| set SP

	| copy down frame (16*4 + 2 + 2 + 2 + 4 + 2 = 76 bytes = 19 longs)
	moveq	#19-1,%d0
Lcpfr:	movel	%a0@+,%a1@+
	dbra	%d0,Lcpfr

	movew	%d1,%sp@(16*4+2)	| set stack adjust count
	movel	%sp@,%sp@-		| push syscall no (original d0 value)
	jbsr	_C_LABEL(syscall)	| re-enter syscall()
	addql	#4,%sp			| pop syscall no
#ifdef DEBUG
	tstw	%sp@(16*4+2)		| stack adjust must be zero
	jeq	Ladjzero
	PANIC("reenter_syscall")
Ladjzero:
#endif
	moveal	%sp@(15*4),%a0		| grab and restore
	movel	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore user registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| rte

