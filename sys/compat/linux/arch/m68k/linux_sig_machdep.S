/*	$NetBSD: linux_sig_machdep.S,v 1.1 1998/12/15 19:25:40 itohy Exp $	*/

/*
 * Written by ITOH Yasufumi.
 * Public domain.
 */

#include <m68k/asm.h>
#include <compat/linux/linux_syscall.h>

/*
 * __dead void linux_reenter_syscall(struct frame *fp, int stkadj)
 *					__attribute__((__noreturn__));
 *
 * Move stack frame by stkadj bytes and re-enter syscall().
 *
 * XXX This is a kludge.
 */

ENTRY_NOPROFILE(linux_reenter_syscall)
	addql	#4,sp			| pop PC
	movel	sp@+,a0			| current frame addr
	movel	sp@,d1			| stkadj

| The m68k frame (struct trapframe) format:
|	16:l	d0-d7/a0-a6/usp
|	1:w	(pad)
|	1:w	stkadj
|	1:w	sr
|	1:l	pc
|	1:w	format/vector

	moveal	a0,a1
	subal	d1,a1			| new frame address
	moveal	a1,sp			| set SP

	| copy down frame (16*4 + 2 + 2 + 2 + 4 + 2 = 76 bytes = 19 longs)
	moveq	#19-1,d0
Lcpfr:	movel	a0@+,a1@+
	dbra	d0,Lcpfr

	movew	d1,sp@(16*4+2)		| set stack adjust count
	movel	sp@,sp@-		| push syscall no (original d0 value)
	jbsr	_C_LABEL(syscall)	| re-enter syscall()
	addql	#4,sp			| pop syscall no
#ifdef DEBUG
	tstw	sp@(16*4+2)		| stack adjust must be zero
	jeq	Ladjzero
	PANIC("linux_reenter_syscall")
Ladjzero:
#endif
	moveal	sp@(15*4),a0		| grab and restore
	movel	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| restore user registers
	addql	#8,sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| rte

/*
 * Signal "trampoline" code for Linux emulation.
 * The sigtramp for Linux/m68k is on the stack frame, and this is a dummy.
 */
	.align	2
GLOBAL(linux_sigcode)
	moveq	#LINUX_SYS_exit,d0	| _exit(?)
	trap	#0
	.align	2
GLOBAL(linux_esigcode)
