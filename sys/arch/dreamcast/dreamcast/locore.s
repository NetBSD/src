/*	$NetBSD: locore.s,v 1.13 2002/02/22 19:45:33 uch Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1997
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */

#include "opt_ddb.h"
#include "opt_lockdebug.h"

#include "assym.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/asm.h>
#include <machine/cputypes.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/trap.h>

#define INIT_STACK	IOM_RAM_BEGIN + IOM_RAM_SIZE - 0x00001000

#ifdef SH4
#define SHREG_BBRA	0xff200008
#define SHREG_CCR	0xff00001c
#define SHREG_EXPEVT	0xff000024
#define SHREG_INTEVT	0xff000028
#define SHREG_MMUCR	0xff000010
#define SHREG_TTB	0xff000008
#else
#define SHREG_BBRA	0xffffffb8
#define SHREG_EXPEVT	0xffffffd4
#define SHREG_INTEVT	0xffffffd8
#define SHREG_MMUCR	0xffffffe0
#define SHREG_TTB	0xfffffff8
#endif

/*
 * These are used on interrupt or trap entry or exit.
 */
#define	INTRENTRY \
	mov	#0x20, r0	; \
	swap.b	r0, r0		; \
	swap.w	r0, r0		; \
	not	r0, r0		; \
	stc	sr, r1		; \
	and	r0, r1		; \
	ldc	r1, sr		; /* Change Register Bank to 0 */ \
	ldc	r8, r0_bank	; \
	ldc	r9, r1_bank	; \
	ldc	r10, r3_bank	; \
	mov	#30, r10	; \
	mov	#2, r9		; \
	shld	r10, r9		; /* r9 = 0x80000000 */ \
	and	r15, r9		; \
	xor	r8, r8		; \
	cmp/eq	r8, r9		; \
	bf	1f		; /* If already kernel mode then jump */ \
	ldc	r15, r2_bank	; \
	mov.l	3f, r8		; /* 3f = Kernel Stack */ \
	mov.l	@r8, r15	; /* Change to Kernel Stack */ \
	stc	r2_bank, r8	; /* r8 = user sp */ \
	mov.l	r8, @-r15	; /* save user sp to stack */ \
	bra	2f		; \
	nop			; \
	.align	2		; \
3:	.long	KernelSp	; \
1:	ldc	r0, r2_bank	; \
	mov	r15, r0		; \
	mov.l	r0, @-r15	; /* save r15 to stack */ \
	stc	r2_bank, r0	; \
2:				; \
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov.l	r2, @-r15	; \
	mov.l	r3, @-r15	; \
	mov.l	r4, @-r15	; \
	mov.l	r5, @-r15	; \
	mov.l	r6, @-r15	; \
	mov.l	r7, @-r15	; \
	stc.l	r0_bank, @-r15	; /* save r8 */ \
	stc.l	r1_bank, @-r15	; /* save r9 */ \
	stc.l	r3_bank, @-r15	; /* save r10 */ \
	mov.l	r11, @-r15	; \
	mov.l	r12, @-r15	; \
	mov.l	r13, @-r15	; \
	mov.l	r14, @-r15	; \
	sts.l	pr, @-r15	; \
	sts.l	mach, @-r15	; \
	sts.l	macl, @-r15	; \
	stc.l	ssr, @-r15	; \
	stc.l	spc, @-r15	; \
	mov.l	r0, @-r15

#define	INTRFASTEXIT \
	add	#4, r15		; /* pop trap event code */ \
	mov.l	@r15+, r0	; \
	ldc	r0, spc		; \
	mov.l	@r15+, r0	; \
	ldc	r0, ssr		; \
	lds.l	@r15+, macl	; \
	lds.l	@r15+, mach	; \
	lds.l	@r15+, pr	; \
	mov.l	@r15+, r14	; \
	mov.l	@r15+, r13	; \
	mov.l	@r15+, r12	; \
	mov.l	@r15+, r11	; \
	mov.l	@r15+, r10	; \
	mov.l	@r15+, r9	; \
	mov.l	@r15+, r8	; \
	mov.l	@r15+, r7	; \
	mov.l	@r15+, r6	; \
	mov.l	@r15+, r5	; \
	mov.l	@r15+, r4	; \
	mov.l	@r15+, r3	; \
	mov.l	@r15+, r2	; \
	mov.l	@r15+, r1	; \
	mov.l	@r15+, r0	; \
	mov.l	@r15, r15	; \
	rte			; \
	nop

#define	RECURSEENTRY \
	mov	r15, r0		; \
	mov.l	r0, @-r15	; /* save r15 to stack */ \
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov.l	r2, @-r15	; \
	mov.l	r3, @-r15	; \
	mov.l	r4, @-r15	; \
	mov.l	r5, @-r15	; \
	mov.l	r6, @-r15	; \
	mov.l	r7, @-r15	; \
	mov.l	r8, @-r15	; /* save r8 */ \
	mov.l	r9, @-r15	; /* save r9 */ \
	mov.l	r10, @-r15	; /* save r10 */ \
	mov.l	r11, @-r15	; \
	mov.l	r12, @-r15	; \
	mov.l	r13, @-r15	; \
	mov.l	r14, @-r15	; \
	sts.l	pr, @-r15	; \
	sts.l	mach, @-r15	; \
	sts.l	macl, @-r15	; \
	stc.l	ssr, @-r15	; \
	stc.l	spc, @-r15	; \
	mov.l	r0, @-r15

/*
 * Macros to disable and enable exceptions (inclding interrupts).
 * This modifies SR.BL
 */
#define EXCEPT_DISABLE		  \
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov	#0x10, r0	; \
	swap.b	r0, r0		; \
	swap.w	r0, r0		; /* r0 = 0x10000000 */ \
	stc	sr, r1		; \
	or	r0, r1		; \
	ldc	r1, sr		; /* disable exceptions */ \
	mov.l	@r15+, r1	; \
	mov.l	@r15+, r0

#define EXCEPT_ENABLE		  \
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov	#0x10, r0	; \
	swap.b	r0, r0		; \
	swap.w	r0, r0		; /* r0 = 0x10000000 */ \
	not	r0, r0		; \
	stc	sr, r1		; \
	and	r0, r1		; \
	ldc	r1, sr		; /* enable exceptions */ \
	mov.l	@r15+, r1	; \
	mov.l	@r15+, r0

/*
 * Macros to disable and enable interrupts.
 * This modifies SR.I[0-3]
 */
#define	INTR_DISABLE		  \
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov	#0x78, r0	; \
	shll	r0		; \
	stc	sr, r1		; \
	or	r0, r1		; \
	ldc	r1, sr		; /* disable interrupt */ \
	mov.l	@r15+, r1	; \
	mov.l	@r15+, r0

#define INTR_ENABLE		  \
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov	#0x78, r0	; \
	shll	r0		; \
	not	r0, r0		; \
	stc	sr, r1		; \
	and	r0, r1		; \
	ldc	r1, sr		; /* enable interrupt */ \
	mov.l	@r15+, r1	; \
	mov.l	@r15+, r0

/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 *
 * XXX 4 == sizeof pde
 */
	.set	_C_LABEL(PTmap), (PDSLOT_PTE << PDSHIFT)
	.set	_C_LABEL(PTD), (_C_LABEL(PTmap) + PDSLOT_PTE * NBPG)
	.set	_C_LABEL(PTDpde), (_C_LABEL(PTD) + PDSLOT_PTE * 4)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 *
 * XXX 4 == sizeof pde
 */
	.set	_C_LABEL(APTmap), (PDSLOT_APTE << PDSHIFT)
	.set	_C_LABEL(APTD), (_C_LABEL(APTmap) + PDSLOT_APTE * NBPG)
	.set	_C_LABEL(APTDpde), (_C_LABEL(PTD) + PDSLOT_APTE * 4)

/*
 * Initialization
 */
	.data
	.globl	_C_LABEL(curpcb), _C_LABEL(PTDpaddr)
_C_LABEL(PTDpaddr):
		.long	0	/* paddr of PTD, for libkvm */
KernelStack:	.long	0
KernelSp:	.long	0	/* Cache for kernel stack pointer of
				   current task */

	.text
	.globl	_C_LABEL(kernel_text)
	.globl	start, _C_LABEL(_start)
	.set	_C_LABEL(kernel_text), KERNTEXTOFF
	.set	_C_LABEL(_start), start

start:
	/* Set SP to initial position */
	mov.l	XLtmpstk, r15

	INTR_DISABLE

	/* Set Register Bank to Bank 0 */
	mov.l	SR_init, r0
	ldc	r0, sr

	xor	r0, r0
	mov.l	XL_SHREG_MMUCR, r2
	mov.l	r0, @r2		/* MMU OFF */

	bra	start1
	nop
#if 0
	mov	#0x20, r8
	swap.b	r8, r8
	swap.w	r8, r8
	not	r8, r8
	stc	sr, r9
	and	r8, r9
	ldc	r9, sr		 /* Change Register Bank to 0 */
#endif
	.align	2
SR_init:	.long	0x500000F0
XL_SHREG_MMUCR:	.long	SHREG_MMUCR
start1:

#ifdef ROMIMAGE
	/* Initialize BUS State Control Regs. */
	mov.l	_ROM_START, r3
	mov.l	_C_LABEL(ram_start), r4
	sub	r3, r4
	/* Set Bus State Controler */
	mov.l	XLInitializeBsc, r0
	sub	r4, r0
	jsr	@r0
	nop

	/* Move kernel image from ROM area to RAM area */
	mov.l	___end, r0
	mov.l	___start, r1
	mov.l	_KERNBASE, r2
	sub	r2, r0
	sub	r2, r1
	sub	r1, r0
	add	#4, r0		/* size of bytes to be copied */
	shlr2	r0		/* number of long word */
	mov.l	_ROM_START, r3
	add	r3, r1		/* src address */
	mov.l	___start, r3
	sub	r2, r3
	mov.l	_C_LABEL(ram_start), r4
	add	r4, r3		/* dest address */
1:
	mov.l	@r1+, r4
	mov.l	r4, @r3
	add	#4, r3
	dt	r0		/* decrement and Test */
	bf	1b
	/* kernel image copy end */

	mov.l	LXstart_in_RAM, r0
	jmp	@r0		/* jump to RAM area */
	nop

	.align	2
LXstart_in_RAM:
	.long	start_in_RAM
#else
#ifndef	DONT_INIT_BSC
	/* Set Bus State Controler */
	mov.l	XLInitializeBsc, r0
	jsr	@r0
	nop
#endif
#endif

start_in_RAM:
	mova	1f, r0
	mov	r0, r4
	mov.l	XLinitSH3, r0
	jsr	@r0		/* call initSH3() */
	nop

	.align	2
1:
#if 1
	mov.l	XLKernelStack, r3
	mov.l	r15, @r3
#endif

#ifdef SH4
	/* CCR must be accessed from P2 area */
	mova	cache_on, r0
	mov	r0, r5
	mov.l	XLtoP2, r1
	or	r1, r5
	mova	main_label, r0
	mov	r0, r2
	mov.l	XL_SHREG_CCR, r3
	mov.l	XL_CCRVAL, r4
	jmp	@r5
	nop

	.align	2
cache_on:
	mov.l	r4, @r3 /* Write to CCR */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	jmp @r2
	nop
	
	.align	2
main_label:
#endif
	mov.l	XLmain, r0
	jsr	@r0		/* call main() */
	nop

		.align	2

	.globl	_C_LABEL(ram_start)
#ifndef	DONT_INIT_BSC
XLInitializeBsc:.long	_C_LABEL(InitializeBsc)
#endif
___start:	.long	start
___etext:	.long	etext
___end:		.long	end
XLtmpstk:	.long	INIT_STACK
_KERNBASE:	.long	KERNBASE
_C_LABEL(ram_start):	.long	IOM_RAM_BEGIN
_ROM_START:	.long	IOM_ROM_BEGIN
XLKernelStack:	.long	KernelStack
XLinitSH3:	.long	_C_LABEL(initSH3)
XLmain:		.long	_C_LABEL(main)
XLtoP2:		.long	0x20000000
XL_SHREG_CCR:	.long	SHREG_CCR
#ifdef SH4
#if 1
XL_CCRVAL:	.long	0x0909 /* Operand cache ON */
#else
XL_CCRVAL:	.long	0x0000 /* cache OFF */
#endif
#endif

NENTRY(proc_trampoline)
	mov	r11, r4
	jsr	@r12
	nop
	add	#4, r15		/* pop tf_trapno */
	EXCEPT_DISABLE
	INTRFASTEXIT
	/* NOTREACHED */

/*****************************************************************************/

/*
 * Signal trampoline; copied to top of user stack.
 */
NENTRY(sigcode)
	mov	r15, r0
	mov.l	@r0, r4		/* sig_no param */
	add	#SIGF_HANDLER, r0
	mov.l	@r0, r0
	jsr	@r0
	nop

	mov	r15, r0
	add	#SIGF_SC, r0
	mov.l	r0, @-r15		/* junk to fake return address */
	mov	r0, r4
	mov.l	XLSYS___sigreturn14, r0
	trapa	#0x80			/* enter kernel with args on stack */
	mov.l	XLSYS_exit, r0
	trapa	#0x80			/* exit if sigreturn fails */

	.align	2
XLSYS___sigreturn14:
	.long	SYS___sigreturn14
XLSYS_exit:
	.long	SYS_exit
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):

/*
 * The following primitives manipulate the run queues.
 * _whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */
	.globl	_C_LABEL(sched_whichqs), _C_LABEL(sched_qs), _C_LABEL(panic)

/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 */

ENTRY(idle)
	/* 
	 * When we get here, interrupts are off (via INTR_DISABLE) and
	 * sched_lock is held.
	 */
	EXCEPT_ENABLE

	mov.l	XXLwhichqs, r0
	mov.l	@r0, r0
	mov	r0, r14
	tst	r0, r0
	bf	sw1

#if defined(LOCKDEBUG)
	mov.l	Xsched_unlock, r0
	jsr	@r0
	nop
#endif
	INTR_ENABLE

	sleep

	INTR_DISABLE
#if defined(LOCKDEBUG)
	mov.l	Xsched_lock, r0
	jsr	@r0
	nop
#endif

	bra	_C_LABEL(idle)
	nop

	.align	2
XXLwhichqs:
	.long	_C_LABEL(sched_whichqs)


#define	PUSHALL	\
	mov.l	r0, @-r15	; \
	mov.l	r1, @-r15	; \
	mov.l	r2, @-r15	; \
	mov.l	r3, @-r15	; \
	mov.l	r4, @-r15	; \
	mov.l	r5, @-r15	; \
	mov.l	r6, @-r15	; \
	mov.l	r7, @-r15	; \
	mov.l	r8, @-r15	; \
	mov.l	r9, @-r15	; \
	mov.l	r10, @-r15	; \
	mov.l	r11, @-r15	; \
	mov.l	r12, @-r15	; \
	mov.l	r13, @-r15	; \
	mov.l	r14, @-r15

#define	POPALL \
	mov.l	@r15+, r14	; \
	mov.l	@r15+, r13	; \
	mov.l	@r15+, r12	; \
	mov.l	@r15+, r11	; \
	mov.l	@r15+, r10	; \
	mov.l	@r15+, r9	; \
	mov.l	@r15+, r8	; \
	mov.l	@r15+, r7	; \
	mov.l	@r15+, r6	; \
	mov.l	@r15+, r5	; \
	mov.l	@r15+, r4	; \
	mov.l	@r15+, r3	; \
	mov.l	@r15+, r2	; \
	mov.l	@r15+, r1	; \
	mov.l	@r15+, r0

#define	PUSHALLBUTR0	\
	mov.l	r1, @-r15	; \
	mov.l	r2, @-r15	; \
	mov.l	r3, @-r15	; \
	mov.l	r4, @-r15	; \
	mov.l	r5, @-r15	; \
	mov.l	r6, @-r15	; \
	mov.l	r7, @-r15	; \
	mov.l	r8, @-r15	; \
	mov.l	r9, @-r15	; \
	mov.l	r10, @-r15	; \
	mov.l	r11, @-r15	; \
	mov.l	r12, @-r15	; \
	mov.l	r13, @-r15	; \
	mov.l	r14, @-r15

#define	POPALLBUTR0 \
	mov.l	@r15+, r14	; \
	mov.l	@r15+, r13	; \
	mov.l	@r15+, r12	; \
	mov.l	@r15+, r11	; \
	mov.l	@r15+, r10	; \
	mov.l	@r15+, r9	; \
	mov.l	@r15+, r8	; \
	mov.l	@r15+, r7	; \
	mov.l	@r15+, r6	; \
	mov.l	@r15+, r5	; \
	mov.l	@r15+, r4	; \
	mov.l	@r15+, r3	; \
	mov.l	@r15+, r2	; \
	mov.l	@r15+, r1

#define DIAGNOSTIC 1
#ifdef DIAGNOSTIC
switch_error:
	mova	1f, r0
	mov	r0, r4
	mov.l	2f, r0
	jsr	@r0
	nop

	.align	2
1:	.asciz	"cpu_swicth"
	.align	2
2:	.long	_C_LABEL(panic)
#endif

/*
 * void cpu_switch(struct proc *)
 * Find a runnable process and switch to it.  Wait if necessary.  If the new
 * process is the same as the old one, we short-circuit the context save and
 * restore.
 */
ENTRY(cpu_switch)
	sts.l	pr, @-r15
	mov.l	r8, @-r15
	mov.l	r9, @-r15
	mov.l	r10, @-r15
	mov.l	r11, @-r15
	mov.l	r12, @-r15
	mov.l	r13, @-r15
	mov.l	r14, @-r15
	mov.l	XXLcpl, r0
	mov.l	@r0, r0
	mov.l	r0, @-r15

	mov.l	XXLcurproc, r12
	mov.l	@r12, r12
	tst	r12, r12
	bt	1f

	/* Save stack pointers. */
	mov	r12, r4
	mov.l	XLP_ADDR, r1
	add	r1, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r0, r4		/* r4 = oldCurproc->p_addr */
	mov	#PCB_R15, r1
	add	r1, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	r15, @r0
1:

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spllower()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	xor	r0, r0
	mov.l	XXLcurproc, r1
	mov.l	r0, @r1

#if 0
	/* switch to proc0's stack */
	mov.l	XXLKernelStack, r1
	mov.l	@r1, r1
	mov.l	XXLKernelSp, r0
	mov.l	r1, @r0
#endif

#if defined(LOCKDEBUG)
	/* Release the sched_lock before processing interrupts. */
	mov.l	Xsched_unlock, r0
	jsr	@r0
	nop
#endif
	xor	r0, r0
	mov.l	XXLcpl, r1
	mov.l	r0, @r1			/* spl0() */
	mov.l	XXLXspllower, r0
	jsr	@r0
	nop
	bra	switch_search
	nop

	.align	2
XXLcpl:		.long	_C_LABEL(cpl)
XXLcurproc:	.long	_C_LABEL(curproc)
XXLXspllower:	.long	_C_LABEL(Xspllower)
XXLKernelStack:	.long	KernelStack
XXLKernelSp:	.long	KernelSp

switch_search:
	/*
	 * First phase: find new process.
	 *
	 * Registers:
	 *   r0  - queue head, scratch, then zero
	 *   r13 - queue number
	 *   r14 - cached value of whichqs
	 *   r9  - next process in queue
	 *   r12 - old process
	 *   r8  - new process
	 */

	/* Lock the scheduler. */
	INTR_DISABLE			/* splhigh doesn't do a cli */
#if defined(LOCKDEBUG)
	mov.l	Xsched_lock, r0
	jsr	@r0
	nop
#endif

	/* Wait for new process. */
	mov.l	XXXLwhichqs, r0
	mov.l	@r0, r0
	mov	r0, r14
	bra	1f
	nop

	.align	2
XXXLwhichqs:
	.long	_C_LABEL(sched_whichqs)
1:


#define TESTANDSHIFT \
	tst	r1, r0		; \
	bf	1f		; \
	shll	r1		; \
	add	#1, r2

sw1:	mov	#1, r1
	xor	r2, r2
	TESTANDSHIFT	/* bit 0 */
	TESTANDSHIFT	/* bit 1 */
	TESTANDSHIFT	/* bit 2 */
	TESTANDSHIFT	/* bit 3 */
	TESTANDSHIFT	/* bit 4 */
	TESTANDSHIFT	/* bit 5 */
	TESTANDSHIFT	/* bit 6 */
	TESTANDSHIFT	/* bit 7 */
	TESTANDSHIFT	/* bit 8 */
	TESTANDSHIFT	/* bit 9 */
	TESTANDSHIFT	/* bit 10 */
	TESTANDSHIFT	/* bit 11 */
	TESTANDSHIFT	/* bit 12 */
	TESTANDSHIFT	/* bit 13 */
	TESTANDSHIFT	/* bit 14 */
	TESTANDSHIFT	/* bit 15 */
	TESTANDSHIFT	/* bit 16 */
	TESTANDSHIFT	/* bit 17 */
	TESTANDSHIFT	/* bit 18 */
	TESTANDSHIFT	/* bit 19 */
	TESTANDSHIFT	/* bit 20 */
	TESTANDSHIFT	/* bit 21 */
	TESTANDSHIFT	/* bit 22 */
	TESTANDSHIFT	/* bit 23 */
	TESTANDSHIFT	/* bit 24 */
	TESTANDSHIFT	/* bit 25 */
	TESTANDSHIFT	/* bit 26 */
	TESTANDSHIFT	/* bit 27 */
	TESTANDSHIFT	/* bit 28 */
	TESTANDSHIFT	/* bit 29 */
	TESTANDSHIFT	/* bit 30 */
	TESTANDSHIFT	/* bit 31 */

	bra	_C_LABEL(idle)		/* if none, idle */
	nop

1:	mov.l	XLqs, r0
	mov	r2, r13
	shll2	r2
	shll	r2
	add	r2, r0		/* r0 = &qs[i] */

	mov	r0, r2
	mov	#P_FORW, r1
	add	r1, r2

	ldc	r0, r0_bank	/* save r0 = &qs[i] */
	mov	r2, r4
	mov.l	r1, @-r15
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r15+, r1
	mov	r0, r2

	mov.l	@r2, r8		/* r8 = qs[i].p_forw */

#ifdef DIAGNOSTIC
	stc	r0_bank, r0
	cmp/eq	r0, r8		/* linked to self (i.e. nothing queued)? */
	bf	10f
	mov.l	XL_switch_error, r0
	jmp	@r0
	nop
10:
#endif

	mov	r8, r3
	add	r1, r3

	mov	r3, r4
	mov.l	r2, @-r15
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r15+, r2
	mov	r0, r3

	mov.l	@r3, r9		/* r9 = qs[i].p_forw->p_forw */

	mov.l	r9, @r2		/* qs[i].p_forw = qs[i].p_forw->p_forw */

	mov	r9, r4
	mov	#P_BACK, r2
	add	r2, r4

	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov	r0, r10
	stc	r0_bank, r0

	mov.l	r0, @r10	/* qs[i].p_forw->p_forw->p_back = &qs[i] */

	mov	r0, r11
	sub	r9, r11
	tst	r11, r11
	bf	3f		/* if r0 != r9 then goto 3f */

	mov	#1, r0
	shld	r13, r0
	not	r0, r0
	and	r0, r14
	mov.l	XLwhichqs, r0
	mov.l	r14, @r0
/* #define sh3_debug */
#ifdef sh3_debug
	mova	1f, r0
	mov	r0, r4
	mov	r13, r5
	mov	r14, r6
	mov.l	2f, r0
	jsr	@r0
	nop
	bra	3f
	nop

	.align	2
1:	.asciz	"switch[i=%d,whichqs=0x%0x]\n"
	.align	2
2:	.long	_C_LABEL(printf)
#endif

3:
	xor	r0, r0
	mov.l	XLwant_resched, r1
	mov.l	r0, @r1

#ifdef DIAGNOSTIC
	mov	r8, r4
	add	#P_WCHAN, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r0, r0
	tst	r0, r0
	bt	11f

	mov	r8, r4
	add	#P_STAT, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.b	@r0, r0
	extu.b	r0, r0
	mov	#SRUN, r1
	cmp/eq	r0, r1
	bt	11f

	mov.l	XL_switch_error, r0
	jmp	@r0
	nop

	.align	2
XL_switch_error:
	.long	switch_error
11:
#endif

	/* Isolate process.  XXX Is this necessary? */
	mov	r8, r4
	mov	#P_BACK, r2
	add	r2, r4

	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov	r0, r1

	xor	r0, r0
	mov.l	r0, @r1		/* r8->p_back = 0 */

#if defined(LOCKDEBUG)
	/*
	 * Unlock the sched_lock, but leave interrupts off, for now.
	 */
	mov.l	Xsched_unlock, r0
	jsr	@r0
	nop
#endif

	/* p->p_cpu initialized in fork1() for single-processor */

	/* Process now running on a processor. */
	mov	#P_STAT, r0
	mov	#SONPROC, r1
	mov.b	r1, @(r0, r8)	/* p->p_stat = SONPROC */

	/* Record new process. */
	mov.l	XXXLcurproc, r0
	mov.l	r8, @r0

	/* It's okay to take interrupts here. */
	INTR_ENABLE

	/* Skip context switch if same process. */
	mov	r12, r0		/* r12 = oldCurproc */
	sub	r8, r0		/* r8 = qs[i]->p_forw */
	tst	r0, r0
	bt	switch_return

	/* If old process exited, don't bother. */
	tst	r12, r12
	bt	switch_exited

	/*
	 * Second phase: save old context.
	 *
	 * Registers:
	 *   %eax, %ecx - scratch
	 *   %esi - old process, then old pcb
	 *   %edi - new process
	 */
	mov	r12, r0
	mov.l	XLP_ADDR, r1
	add	r1, r0
	mov.l	@r0, r12	/* r12 = oldCurproc->p_addr */

	/* Save stack pointers. */
	mov	r12, r0
	add	#PCB_R15, r0
	mov.l	r15, @r0

switch_exited:
	/*
	 * Third phase: restore saved context.
	 *
	 * Registers:
	 *   %eax, %ecx, %edx - scratch
	 *   %esi - new pcb
	 *   %edi - new process
	 */

	/* No interrupts while loading new state. */
	INTR_DISABLE
	mov	r8, r4		/* r8 = qs[i]->p_forw */
	mov.l	XLP_ADDR, r1
	add	r1, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r0, r12

	/* Restore stack pointers. */
	mov	r12, r4
	add	#PCB_R15, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r0, r15

	/* Store new kernel mode stack pointer */
	mov	r12, r4
	add	#PCB_KR15, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop
	mov.l	@r0, r0
	mov.l	XL_KernelSp, r1
	mov.l	r0, @r1

	/* Switch address space. */
	mov	r12, r4
	add	#PCB_PAGEDIRREG, r4
	mov.l	XL_ConvVtoP, r0
	jsr	@r0
	nop

	mov.l	@r0, r0
	mov.l	XL_SHREG_TTB, r2
	mov.l	r0, @r2

	/* flush TLB */
	mov.l	XXL_SHREG_MMUCR, r0
	mov	#4, r1
	mov.l	@r0, r2
	or	r1, r2
#ifdef SH4
	mov.l	XL_MMUCR_VBITS, r1
	and	r1, r2
#endif
	mov.l	r2, @r0

switch_restored:
	/* Record new pcb. */
	mov.l	XL_curpcb, r0
	mov.l	r12, @r0

	/* Interrupts are okay again. */
	INTR_ENABLE

switch_return:
	/*
	 * Restore old cpl from stack.  Note that this is always an increase,
	 * due to the spl0() on entry.
	 */
	mov.l	@r15+, r0
	mov.l	XLcpl, r1
	mov.l	r0, @r1

	mov	r8, r0			/* return (p); */
	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	mov.l	@r15+, r8
	lds.l	@r15+, pr

	rts
	nop

	.align	2
XLqs:		.long	_C_LABEL(sched_qs)
XLP_ADDR:	.long	P_ADDR
XLwhichqs:	.long	_C_LABEL(sched_whichqs)
XLwant_resched:	.long	_C_LABEL(want_resched)
XXXLcurproc:	.long	_C_LABEL(curproc)
XL_ConvVtoP:	.long	_C_LABEL(ConvVtoP)
XL_KernelSp:	.long	KernelSp
XL_SHREG_TTB:	.long	SHREG_TTB
#if defined(LOCKDEBUG)
Xsched_lock:	.long	_C_LABEL(sched_lock_idle)
Xsched_unlock:	.long	_C_LABEL(sched_unlock_idle)
#endif
/*
 * switch_exit(struct proc *p);
 * Switch to proc0's saved context and deallocate the address space and kernel
 * stack for p.  Then jump into cpu_switch(), as if we were in proc0 all along.
 */
	.globl	_C_LABEL(proc0), _C_LABEL(kernel_map)
ENTRY(switch_exit)
	mov	r4, r8			/* old process */
	mov.l	XLproc0, r9

	/* In case we fault... */
	xor	r0, r0
	mov.l	XLcurproc, r1
	mov.l	r0, @r1

	/* Restore proc0's context. */
	INTR_DISABLE
	mov	r9, r0
	mov.l	XXLP_ADDR, r1
	add	r1, r0
	mov.l	@r0, r10

	/* Restore stack pointers. */
	mov	r10, r0
	mov	#PCB_R15, r1
	add	r1, r0
	mov.l	@r0, r15

	/* Switch address space. */
	mov	r10, r0
	add	#PCB_PAGEDIRREG, r0
	mov.l	@r0, r2
	mov.l	XXL_SHREG_TTB, r1
	mov.l	r2, @r1

	/* flush TLB */
	mov.l	XXL_SHREG_MMUCR, r0
	mov	#4, r1
	mov.l	@r0, r2
	or	r1, r2
#ifdef SH4
	mov.l	XL_MMUCR_VBITS, r1
	and	r1, r2
#endif
	mov.l	r2, @r0

	/* Record new pcb. */
	mov.l	XL_curpcb, r0
	mov.l	r10, @r0

	/* Interrupts are okay again. */
	INTR_ENABLE

	mov	r8, r4
	mov.l	XLexit2, r0
	jsr	@r0			/* exit2(p) */
	nop

	/* Jump into cpu_switch() with the right state. */
	mov	r9, r12
	xor	r0, r0
	mov.l	XLcurproc, r1
	mov.l	r0, @r1

	bra	switch_search
	nop

	.align	2
	.globl	_C_LABEL(exit2)
XLexit2:
	.long	_C_LABEL(exit2)

#ifdef SH4
XL_MMUCR_VBITS:
	.long	MMUCR_VALIDBITS
#endif

XXL_SHREG_MMUCR:
	.long	SHREG_MMUCR
XXLP_ADDR:
	.long	P_ADDR

/*
 * savectx(struct pcb *pcb);
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	mov.l	r14, @-r15
	sts.l	pr, @-r15
	mov	r15, r14
	add	#PCB_R15, r4
	mov.l	r15, @r4
	mov	r14, r15
	lds.l	@r15+, pr
	mov.l	@r15+, r14

	rts
	nop

/*****************************************************************************/
/*
 * Trap and fault vector routines
 *
 * On exit from the kernel to user mode, we always need to check for ASTs.  In
 * addition, we need to do this atomically; otherwise an interrupt may occur
 * which causes an AST, but it won't get processed until the next kernel entry
 * (possibly the next clock tick).  Thus, we disable interrupt before checking,
 * and only enable them again on the final `iret' or before calling the AST
 * handler.
 *
 * XXX - debugger traps are now interrupt gates so at least bdb doesn't lose
 * control.  The sti's give the standard losing behaviour for ddb and kgdb.
 */

	.text

NENTRY(exphandler)
/* #define	CHECK_SP */
#ifdef CHECK_SP
	mov.l	XL_splimit3, r0
	cmp/hs	r15, r0
	bf	100f
	mov.l	XL_splimit_low3, r0
	cmp/hs	r0, r15
	bf	100f
	xor	r0, r0
	jmp	@r0
	nop
100:
#endif

	mov.l	XL_SHREG_EXPEVT, r0
	mov.l	@r0, r0
	cmp/eq	#0x40, r0	/* T_TLBINVALIDR */
	bf	1f
3:
	mov.l	XL_tlbmisshandler, r0
	jmp	@r0
1:
	cmp/eq	#0x60, r0	/* T_TLBINVALIDW */
	bt	3b

	mov.l	XL_TLBPROTWR, r1
	cmp/eq	r0, r1
	bt	3b

	INTRENTRY
#ifdef DDB
	mov	#0, r0
	mov.l	XL_SHREG_BBRA, r1
	mov.w	r0, @r1		/* disable UBC */
	mov.l	r0, @r15	/* clear frame->dummy */
#endif
	mov.l	XL_SHREG_EXPEVT, r0
	mov.l	@r0, r0
	mov.l	r0, @-r15
	INTR_ENABLE
	EXCEPT_ENABLE
	mov.l	XL_trap, r0
	jsr	@r0
	nop
	add	#4, r15
	nop

2:	/* Check for ASTs on exit to user mode. */
	INTR_DISABLE
	mov.l	XL_astpending, r0
	mov.l	@r0, r0
	tst	r0, r0
	bt	1f

	/* If trap occurred in kernel , skip AST proc */
	mov	r15, r0
	add	#TF_SPC-4, r0
	mov.l	@r0, r0
	mov	#1, r1
	mov	#31, r2
	shld	r2, r1
	tst	r1, r0		/* test MSB of TF_SPC */
	bf	1f
5:	xor	r0, r0
	mov.l	XL_astpending, r1
	mov.l	r0, @r1

	INTR_ENABLE

	mov.l	XLT_ASTFLT, r1
	mov.l	r1, @-r15
	mov.l	XL_trap, r0
	jsr	@r0
	nop
	add	#4, r15
	bra	2b
	nop
1:
	EXCEPT_DISABLE

#ifdef DDB
	mov.l	@r15, r0
	mov.l	XL_SHREG_BBRA, r1
	mov.w	r0, @r1
#endif
	INTRFASTEXIT

	.align	2
XL_TLBPROTWR:
	.long	0x000000c0
XL_SHREG_EXPEVT:
	.long	SHREG_EXPEVT
XL_SHREG_BBRA:
	.long	SHREG_BBRA

	.globl	_C_LABEL(tlbmisshandler_stub)
	.globl	_C_LABEL(tlbmisshandler_stub_end)
_C_LABEL(tlbmisshandler_stub):
	mov.l	XL_tlbmisshandler, r0
	jmp	@r0
	nop
	.align	2
XL_tlbmisshandler:
	.long	_C_LABEL(tlbmisshandler)
_C_LABEL(tlbmisshandler_stub_end):

	.align	2
NENTRY(tlbmisshandler)
	INTRENTRY
#ifdef CHECK_SP
	mov.l	XL_splimit3, r0
	cmp/hs	r15, r0
	bf	100f
	mov.l	XL_splimit_low3, r0
	cmp/hs	r0, r15
	bf	100f
	xor	r0, r0
	jmp	@r0
	nop
	.align	2
XL_splimit3:		.long	_end
XL_splimit_low3:	.long	0x80000000
100:
#endif
	INTR_DISABLE
	/* we must permit exceptions to enable address translation */
	EXCEPT_ENABLE
	mov.l	r0, @-r15	/* push dummy trap code */
	mov.l	XL_tlb_handler, r0
	jsr	@r0
	nop
	add	#4, r15		/* pop dummy code */
	EXCEPT_DISABLE
	INTRFASTEXIT

	.align	2
	.globl	_C_LABEL(MonTrap100), _C_LABEL(MonTrap100_end)
_C_LABEL(MonTrap100):
	mov.l	1f, r0
	jmp	@r0
	nop

	.align	2
1:
	.long	_C_LABEL(exphandler)
_C_LABEL(MonTrap100_end):

	.align	2
	.globl	_C_LABEL(MonTrap600), _C_LABEL(MonTrap600_end)
_C_LABEL(MonTrap600):
	mov.l	1f, r0
	jmp	@r0
	nop

	.align	2
1:
	.long	_C_LABEL(ihandler)
_C_LABEL(MonTrap600_end):

/*
 * Immediate Data
 */
		.align	2

XL_curpcb:	.long	_C_LABEL(curpcb)
XLcurproc:	.long	_C_LABEL(curproc)
XLcpl:		.long	_C_LABEL(cpl)
XLXspllower:	.long	_C_LABEL(Xspllower)
XLproc0:	.long	_C_LABEL(proc0)

XL_trap:	.long	_C_LABEL(trap)
XL_astpending:	.long	_C_LABEL(astpending)
XLT_ASTFLT:	.long	T_ASTFLT
XL_tlb_handler:	.long	_C_LABEL(tlb_handler)
XLexphandler:	.long	_C_LABEL(exphandler)

	/*
	 * Convert Virtual address to Physical Address
	 * r4 = Virtual Address
	 * r0 = returned Physical address
	 */
#ifdef SH4
ENTRY(ConvVtoP)
	sts.l	pr, @-r15
	mov.l	r4, @-r15
	mov.l	XL_cacheflush, r0
	jsr	@r0
	nop
	mov.l	@r15+, r4
	mov	r4, r0
	mov.l	XL_CSMASK, r1
	mov.l	XL_KCSAREA, r2
	and	r4, r1
	cmp/eq	r1, r2
	bt	1f
	mov.l	XL_P2AREA, r5
	mov	#PDSHIFT, r1
	neg	r1, r1
	shld	r1, r0
	shll2	r0
	mov.l	XXL_SHREG_TTB, r1
	or	r5, r1	
	mov.l	@r1, r1
	add	r0, r1
	or	r5, r1
	mov.l	@r1, r2		/* r2 = pde  */
	mov.l	XL_PG_FRAME, r1
	and	r1, r2		/* r2 = page table address */
	mov	r4, r0
	mov.l	XL_PT_MASK, r3
	and	r3, r0
	mov	#PGSHIFT, r1
	neg	r1, r1
	shld	r1, r0
	shll2	r0
	add	r0, r2		/* r2 = pte */
	or	r5, r2
	mov.l	@r2, r2
	mov.l	XL_PG_FRAME, r1
	and	r1, r2
	not	r1, r1
	and	r1, r4
	or	r4, r2
	or	r5, r2
	mov	r2, r0		/* r0 = P2 address */
1:
	lds.l	@r15+, pr
	rts
	nop
#else /* SH4 */
ENTRY(ConvVtoP)
	mov	r4, r0
	mov.l	XL_CSMASK, r1
	mov.l	XL_KCSAREA, r2
	and	r4, r1
	cmp/eq	r1, r2
	bt	1f
	mov	#PDSHIFT, r1
	neg	r1, r1
	shld	r1, r0
	shll2	r0
	mov.l	XXL_SHREG_TTB, r1
	mov.l	@r1, r1
	add	r0, r1
	mov.l	@r1, r2		/* r2 = pde */
	mov.l	XL_PG_FRAME, r1
	and	r1, r2		/* r2 = page table address */
	mov	r4, r0
	mov.l	XL_PT_MASK, r3
	and	r3, r0
	mov	#PGSHIFT, r1
	neg	r1, r1
	shld	r1, r0
	shll2	r0
	add	r0, r2		/* r2 = pte */
	mov.l	@r2, r2
	mov.l	XL_PG_FRAME, r1
	and	r1, r2
	not	r1, r1
	and	r1, r4
	or	r4, r2
	mov	r2, r0		/* r0 = P1 address */
1:
	rts
	nop
#endif /* SH4 */

	.align	2
XL_PT_MASK:	.long	PT_MASK
XL_PG_FRAME:	.long	PG_FRAME
XL_CSMASK:	.long	0xc0000000
XL_KCSAREA:	.long	0x80000000
XXL_SHREG_TTB:	.long	SHREG_TTB
XL_P2AREA:	.long	0xa0000000
#ifdef SH4
XL_cacheflush:	.long	_C_LABEL(sh4_cache_flush)
#endif

Xrecurse:
	stc	sr, r0
	ldc	r0, ssr
	ldc	r1, spc
	RECURSEENTRY
	bra	7f
	nop

NENTRY(ihandler)
	INTRENTRY
#ifdef CHECK_SP
	mov.l	XL_splimit2, r0
	cmp/hs	r15, r0
	bf	100f
	mov.l	XL_splimit_low2, r0
	cmp/hs	r0, r15
	bf	100f
	xor	r0, r0
	jmp	@r0
	nop
	.align	2
XL_splimit2:		.long	_end
XL_splimit_low2:	.long	0x80000000
100:
#endif
7:
	mov.l	XL_SHREG_INTEVT, r0
	mov.l	@r0, r0
	mov.l	r0, @-r15
6:
	INTR_DISABLE		/* disable external interrupt */
	EXCEPT_ENABLE		/* enable exception for TLB handling */
	mov.l	XL_intrhandler, r0
	jsr	@r0
	nop
	add	#4, r15
	nop

	tst	r0, r0
	bt	1f

	mov.l	XL_check_ipending, r0
	jsr	@r0
	nop

	tst	r0, r0
	bf	7b

2:	/* Check for ASTs on exit to user mode. */
	INTR_DISABLE
	mov.l	XXL_astpending, r0
	mov.l	@r0, r0
	tst	r0, r0
	bt	1f

	/* If trap occurred in kernel , skip AST proc */
	mov	r15, r0
	add	#TF_SPC-4, r0
	mov.l	@r0, r0
	mov	#1, r1
	mov	#31, r2
	shld	r2, r1
	tst	r1, r0		/* test MSB of TF_SPC */
	bf	1f
5:	xor	r0, r0
	mov.l	XXL_astpending, r1
	mov.l	r0, @r1
	INTR_ENABLE
	mov.l	XXLT_ASTFLT, r1
	mov.l	r1, @-r15
	mov.l	XXL_trap, r0
	jsr	@r0
	nop
	add	#4, r15
	bra	2b
	nop
1:
	EXCEPT_DISABLE
	INTRFASTEXIT

	.align	2
XL_SHREG_INTEVT:	.long	SHREG_INTEVT
XL_intrhandler:		.long	_C_LABEL(intrhandler)
XXL_astpending:		.long	_C_LABEL(astpending)
XXLT_ASTFLT:		.long	T_ASTFLT
XXL_trap:		.long	_C_LABEL(trap)
XL_check_ipending:	.long	_C_LABEL(check_ipending)

NENTRY(Xspllower)
	sts.l	pr, @-r15
	mov.l	r1, @-r15

Xrestart:
	INTR_DISABLE
	EXCEPT_ENABLE
	mov.l	XXL_check_ipending, r0
	jsr	@r0
	nop

	tst	r0, r0
	bt	1f

	mov.l	XL_restart, r1
	mov.l	XL_Xrecurse, r0
	jmp	@r0
	nop

1:
	INTR_ENABLE
	mov.l	@r15+, r1
	lds.l	@r15+, pr
	rts
	nop

	.align	2
XXL_check_ipending:
		.long	_C_LABEL(check_ipending)
XL_Xrecurse:	.long	Xrecurse
XL_restart:	.long	Xrestart

ENTRY(cpu_printR15)
	sts.l	pr, @-r15
	mova	1f, r0
	mov	r0, r4
	mov	r15, r5
	mov.l	2f, r0
	jsr	@r0
	nop
	lds.l	@r15+, pr
	rts
	nop

	.align	2
1:	.asciz	"sp=0x%x\n"
	.align	2
2:	.long	_C_LABEL(printf)

load_and_reset:
	mov.l	XL_start_address, r0
	mov	r0, r8
	mov.l	@r4+, r1	/* r1 = osimage size */
	mov.l	@r4+, r2	/* r2 = check sum */
	shlr2	r1		/* r1 = osimage size in dword */
1:
	mov.l	@r4+, r3
	mov.l	r3, @r0
	add	#4, r0
	dt	r1
	bf	1b

	jmp	@r8		/* jump to start address */
	nop

	.align	2
XL_start_address:
	.long	IOM_RAM_BEGIN + 0x00010000
load_and_reset_end:

ENTRY(XLoadAndReset)
	INTR_DISABLE
	/* copy trampoline code to RAM area top */
	mov.l	XL_load_and_reset, r0
	mov.l	XL_load_and_reset_end, r1
	mov.l	XL_load_trampoline_addr, r2
	mov	r2, r8
	sub	r0, r1		/* r1 = bytes to be copied */
1:	mov.b	@r0+, r3
	mov.b	r3, @r2
	add	#1, r2
	dt	r1
	bf	1b

	jmp	@r8		/* jump to trampoline code */
	nop

	.align	2
XL_load_trampoline_addr:
	.long	IOM_RAM_BEGIN + 0x00008000
XL_load_and_reset:
	.long	load_and_reset
XL_load_and_reset_end:
	.long	load_and_reset_end

ENTRY(Sh3Reset)
	mov.l	XL_reset_vector, r8
	jmp	@r8
	nop

	.align	2
XL_reset_vector:
	.long	0xa0000000

	.data
	.align	2
	.globl	_C_LABEL(intrcnt), _C_LABEL(eintrcnt)
	.globl	_C_LABEL(intrnames), _C_LABEL(eintrnames)
_C_LABEL(intrcnt):
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
_C_LABEL(eintrcnt):

_C_LABEL(intrnames):
	.asciz	"irq0", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
_C_LABEL(eintrnames):
