/*	$NetBSD: locore_subr.S,v 1.5.4.2 2002/03/16 15:59:42 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
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
#include "opt_kgdb.h"
	
#include "assym.h"	

#include <sh3/asm.h>
#include <sh3/locore.h>
	
#include <sys/syscall.h>/* SYS___sigreturn14, SYS_exit */	
#include <sh3/trap.h>	/* T_ASTFLT */	
#include <sh3/param.h>	/* NBPG */
#include <sh3/pte.h>	/* PDSHIFT */
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>
	
	.text
	.align 5	/* align cache line size (32B) */

/*
 * u_int32_t _cpu_intr_suspend(void):
 *	Mask all external interrupt. returns previous mask. (SR.IMASK)
 */					
NENTRY(_cpu_intr_suspend)
	stc	sr, r0		/* r0 = SR */	
	mov	#0x78, r1
	shll	r1		/* r1 = 0x000000f0 */
	mov	r0, r2		/* r2 = SR */
	or	r1, r2		/* r2 |= 0x000000f0 */
	ldc	r2, sr		/* SR = r2 */
	rts
	and	r1, r0		/* r0 = SR & 0x000000f0 */

/*
 * void _cpu_intr_resume(u_int32_t s):
 *	restore 's' interrupt mask. (SR.IMASK)
 */
NENTRY(_cpu_intr_resume)
	stc	sr, r0		/* r0 = SR */	
	mov	#0x78, r1
	shll	r1		/* r1 = 0x000000f0 */
	not	r1, r1		/* r1 = 0xffffff0f */
	and	r1, r0		/* r0 &= ~0x000000f0 */
	or	r4, r0		/* r0 |= old SR.IMASK */
	ldc	r0, sr		/* SR = r0 (don't move to delay slot) */
	rts
	nop	

/*	
 * u_int32_t _cpu_exception_suspend(void):
 *	Block exception (SR.BL). if external interrupt raise, pending interrupt.
 *	if exception occur, jump to 0xa0000000 (hard reset).
 */	
NENTRY(_cpu_exception_suspend)	
	stc	sr, r0		/* r0 = SR */
	mov	#0x10, r1
	swap.b	r1, r1
	mov	r0, r2		/* r2 = r0 */
	swap.w	r1, r1		/* r1 = 0x10000000 */
	or	r1, r2		/* r2 |= 0x10000000 */
	ldc	r2, sr		/* SR = r2 */
	rts
	and	r1, r0		/* r0 &= 0x10000000 */

/*
 * void _cpu_exception_resume(u_int32_t s):
 *	restore 's' exception mask. (SR.BL)
 */
NENTRY(_cpu_exception_resume)	
	stc	sr, r0		/* r0 = SR */	
	mov	#0x10, r1
	swap.b	r1, r1
	swap.w	r1, r1
	not	r1, r1		/* r1 = ~0x10000000 */
	and	r1, r0		/* r0 &= ~0x10000000 */
	or	r4, r0		/* r0 |= old SR.BL */
	ldc	r0, sr		/* SR = r0 (don't move to delay slot) */	
	rts
	nop

/*
 * void _cpu_spin(u_int32_t count)
 *	loop 'count' * 10 cycle.
 * [...]
 * add    IF ID EX MA WB
 * nop       IF ID EX MA WB
 * cmp/pl       IF ID EX MA WB -  -
 * nop             IF ID EX MA -  -  WB
 * bt                 IF ID EX .  .  MA WB
 * nop                   IF ID -  -  EX MA WB
 * nop                      IF -  -  ID EX MA WB
 * nop                      -  -  -  IF ID EX MA WB
 * add                                  IF ID EX MA WB
 * nop                                     IF ID EX MA WB
 * cmp/pl                                     IF ID EX MA WB -  -
 * nop                                           IF ID EX MA -  - WB
 * bt                                               IF ID EX .  . MA
 * [...]
 */		
	.align 5	/* align cache line size (32B) */
NENTRY(_cpu_spin)
1:	nop			/* 1 */
	nop			/* 2 */
	nop			/* 3 */
	add	#-1, r4		/* 4 */
	nop			/* 5 */
	cmp/pl	r4		/* 6 */
	nop			/* 7 */
	bt	1b		/* 8, 9, 10 */
	rts
	nop

#if defined(DDB) || defined(KGDB)
/*	
 * int setjmp(label_t *):
 */		
ENTRY(setjmp)
	add	#4*9, r4
	mov.l	r8,  @-r4
	mov.l	r9,  @-r4
	mov.l	r10, @-r4
	mov.l	r11, @-r4
	mov.l	r12, @-r4
	mov.l	r13, @-r4
	mov.l	r14, @-r4
	mov.l	r15, @-r4
	sts.l	pr,  @-r4
	rts
	xor	r0, r0
/*
 * void longjmp(label_t *):
 */		
ENTRY(longjmp)
	lds.l	@r4+, pr
	mov.l	@r4+, r15
	mov.l	@r4+, r14
	mov.l	@r4+, r13
	mov.l	@r4+, r12
	mov.l	@r4+, r11
	mov.l	@r4+, r10
	mov.l	@r4+, r9
	mov.l	@r4+, r8
	rts
	nop
#endif /* DDB || KGDB */

/* 	
 * Call the service funciton with one argument specified by the r12 and r11
 * respectively.
 */	
NENTRY(proc_trampoline)
	jsr	@r12
	 mov	r11, r4
	EXCEPTION_RETURN
	/* NOTREACHED */

/*
 * Signal trampoline; copied to top of user stack.
 */
	.globl	_C_LABEL(esigcode)	
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
	mov.l	_L.SYS___sigreturn14, r0
	trapa	#0x80			/* enter kernel with args on stack */
	mov.l	_L.SYS_exit, r0
	trapa	#0x80			/* exit if sigreturn fails */

	.align	2
_L.SYS___sigreturn14:	.long	SYS___sigreturn14
_L.SYS_exit:		.long	SYS_exit
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
/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 */
ENTRY(idle)
	/* 
	 * When we get here, interrupts are off (via INTR_DISABLE) and
	 * sched_lock is held.
	 */
	mov.l	X_L.sched_whichqs, r0
	mov.l	@r0, r0
	mov	r0, r14
	tst	r0, r0
	bf	sw1

#if defined(LOCKDEBUG)
	mov.l	_L.sched_unlock, r0
	jsr	@r0
	 nop
#endif
	__INTR_UNMASK(r0, r1)

	sleep

	__INTR_MASK(r0, r1)
#if defined(LOCKDEBUG)
	mov.l	_L.sched_lock, r0
	jsr	@r0
	 nop
#endif
	bra	_C_LABEL(idle)
	 nop
	.align	2
X_L.sched_whichqs:	.long	_C_LABEL(sched_whichqs)

/*
 * void cpu_switch(struct proc *)
 * Find a runnable process and switch to it.  Wait if necessary.  If the new
 * process is the same as the old one, we short-circuit the context save and
 * restore.
 */
#ifdef DIAGNOSTIC
switch_error:
	mova	1f, r0
	mov	r0, r4
	mov.l	2f, r0
	jsr	@r0
	 nop

	.align	2
1:	.asciz	"cpu_switch"
	.align	2
2:	.long	_C_LABEL(panic)
#endif /* DIAGNOSTIC */
ENTRY(cpu_switch)
	sts.l	pr, @-r15
	mov.l	r8, @-r15
	mov.l	r9, @-r15
	mov.l	r10, @-r15
	mov.l	r11, @-r15
	mov.l	r12, @-r15
	mov.l	r13, @-r15
	mov.l	r14, @-r15
	mov.l	X_L.cpl, r0
	mov.l	@r0, r0
	mov.l	r0, @-r15

	mov.l	X_L.curproc, r12
	mov.l	@r12, r12
	tst	r12, r12
	bt	1f

	/* Save stack pointers. */
	mov	r12, r4
	mov.l	_L.P_ADDR, r1
	add	r1, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.l	@r0, r4		/* r4 = oldCurproc->p_addr */
	mov	#PCB_R15, r1
	add	r1, r4
	MOV	(mmu_pt_kaddr, r0)
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
	mov.l	X_L.curproc, r1
	mov.l	r0, @r1

#if defined(LOCKDEBUG)
	/* Release the sched_lock before processing interrupts. */
	mov.l	_L.sched_unlock, r0
	jsr	@r0
	 nop
#endif
	xor	r0, r0
	mov.l	X_L.cpl, r1
	mov.l	r0, @r1			/* spl0() */
	mov.l	X_L.Xspllower, r0
	jsr	@r0
	 nop
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
	__INTR_MASK(r0, r1)
#if defined(LOCKDEBUG)
	mov.l	_L.sched_lock, r0
	jsr	@r0
	 nop
#endif

	/* Wait for new process. */
	mov.l	_L.sched_whichqs, r0
	mov.l	@r0, r0
	mov	r0, r14

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

1:	mov.l	_L.sched_qs, r0
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
	MOV	(mmu_pt_kaddr, r0)
	jsr	@r0
	 nop
	mov.l	@r15+, r1
	mov	r0, r2

	mov.l	@r2, r8		/* r8 = qs[i].p_forw */

#ifdef DIAGNOSTIC
	stc	r0_bank, r0
	cmp/eq	r0, r8		/* linked to self (i.e. nothing queued)? */
	bf	10f
	mov.l	_L._switch_error, r0
	jmp	@r0
	 nop
10:
#endif /* DIAGNOSTIC */

	mov	r8, r3
	add	r1, r3

	mov	r3, r4
	mov.l	r2, @-r15
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.l	@r15+, r2
	mov	r0, r3

	mov.l	@r3, r9		/* r9 = qs[i].p_forw->p_forw */

	mov.l	r9, @r2		/* qs[i].p_forw = qs[i].p_forw->p_forw */

	mov	r9, r4
	mov	#P_BACK, r2
	add	r2, r4

	MOV	(mmu_pt_kaddr, r0)
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
	mov.l	_L.sched_whichqs, r0
	mov.l	r14, @r0
#ifdef CONTEXT_SWITCH_DEBUG
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
#endif /* CONTEXT_SWITCH_DEBUG */

3:
	xor	r0, r0
	mov.l	_L.want_resched, r1
	mov.l	r0, @r1

#ifdef DIAGNOSTIC
	mov	r8, r4
	add	#P_WCHAN, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.l	@r0, r0
	tst	r0, r0
	bt	11f

	mov	r8, r4
	add	#P_STAT, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.b	@r0, r0
	extu.b	r0, r0
	mov	#SRUN, r1
	cmp/eq	r0, r1
	bt	11f

	mov.l	_L._switch_error, r0
	jmp	@r0
	 nop

	.align	2
_L._switch_error:
	.long	switch_error
11:
#endif /* DIAGNOSTIC */

	/* Isolate process.  XXX Is this necessary? */
	mov	r8, r4
	mov	#P_BACK, r2
	add	r2, r4

	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov	r0, r1

	xor	r0, r0
	mov.l	r0, @r1		/* r8->p_back = 0 */

#if defined(LOCKDEBUG)
	/*
	 * Unlock the sched_lock, but leave interrupts off, for now.
	 */
	mov.l	_L.sched_unlock, r0
	jsr	@r0
	 nop
#endif

	/* p->p_cpu initialized in fork1() for single-processor */

	/* Process now running on a processor. */
	mov	#P_STAT, r0
	mov	#SONPROC, r1
	mov.b	r1, @(r0, r8)	/* p->p_stat = SONPROC */

	/* Record new process. */
	mov.l	_L.curproc, r0
	mov.l	r8, @r0

	/* It's okay to take interrupts here. */
	__INTR_UNMASK(r0, r1)

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
	 */
	mov	r12, r0
	mov.l	_L.P_ADDR, r1
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
	 */

	/* No interrupts while loading new state. */
	__INTR_MASK(r0, r1)
	mov	r8, r4		/* r8 = qs[i]->p_forw */
	mov.l	_L.P_ADDR, r1
	add	r1, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.l	@r0, r12

	/* Restore stack pointers. */
	mov	r12, r4
	add	#PCB_R15, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.l	@r0, r15

	/* Store new kernel mode stack pointer */
	mov	r12, r4
	add	#PCB_KR15, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop
	mov.l	@r0, r0
	ldc	r0, r7_bank	/* set kernel stack pointer */

	/* Switch address space. */
	mov	r12, r4
	add	#PCB_PAGEDIRREG, r4
	MOV	(mmu_pt_kaddr, r0)	
	jsr	@r0
	 nop

	mov.l	@r0, r0
	MOV	(TTB, r2)
	mov.l	r0, @r2

	/* flush TLB */
	MOV	(tlb_invalidate_all, r0)
	jsr	@r0
	 nop

switch_restored:
	/* Record new pcb. */
	mov.l	_L.curpcb, r0
	mov.l	r12, @r0

	/* Interrupts are okay again. */
	__INTR_UNMASK(r0, r1)

switch_return:
	/*
	 * Restore old cpl from stack.  Note that this is always an increase,
	 * due to the spl0() on entry.
	 */
	mov.l	@r15+, r0
	mov.l	_L.cpl, r1
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
X_L.cpl:		.long	_C_LABEL(cpl)
X_L.curproc:		.long	_C_LABEL(curproc)
X_L.Xspllower:		.long	_C_LABEL(Xspllower)
_L.sched_qs:		.long	_C_LABEL(sched_qs)
_L.sched_whichqs:	.long	_C_LABEL(sched_whichqs)
_L.want_resched:	.long	_C_LABEL(want_resched)
#if defined(SH3) && defined(SH4)
_L.mmu_pt_kaddr:	.long	_C_LABEL(__sh_mmu_pt_kaddr)
#elif defined(SH3)	
_L.mmu_pt_kaddr:	.long	_C_LABEL(sh3_mmu_pt_p1addr)
#elif defined(SH4)
_L.mmu_pt_kaddr:	.long	_C_LABEL(sh4_mmu_pt_p2addr)
#endif
#if defined(LOCKDEBUG)
_L.sched_lock:		.long	_C_LABEL(sched_lock_idle)
_L.sched_unlock:	.long	_C_LABEL(sched_unlock_idle)
#endif

/*
 * switch_exit(struct proc *p);
 * Switch to proc0's saved context and deallocate the address space and kernel
 * stack for p.  Then jump into cpu_switch(), as if we were in proc0 all along.
 */
ENTRY(switch_exit)
	mov	r4, r8			/* old process */
	mov.l	_L.proc0, r9

	/* In case we fault... */
	xor	r0, r0
	mov.l	_L.curproc, r1
	mov.l	r0, @r1

	/* Restore proc0's context. */
	__INTR_MASK(r0, r1)
	mov	r9, r0
	mov.l	_L.P_ADDR, r1
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
	MOV	(TTB, r1)
	mov.l	r2, @r1

	/* flush TLB */
	MOV	(tlb_invalidate_all, r0)	
	jsr	@r0
	 nop
	
	/* Record new pcb. */
	mov.l	_L.curpcb, r0
	mov.l	r10, @r0

	/* Interrupts are okay again. */
	__INTR_UNMASK(r0, r1)

	mov	r8, r4
	mov.l	_L.exit2, r0
	jsr	@r0			/* exit2(p) */
	 nop

	/* Jump into cpu_switch() with the right state. */
	mov	r9, r12
	xor	r0, r0
	mov.l	_L.curproc, r1
	mov.l	r0, @r1

	bra	switch_search
	 nop
	.align	2
_L.P_ADDR:	.long	P_ADDR		
_L.exit2:	.long	_C_LABEL(exit2)
_L.curpcb:	.long	_C_LABEL(curpcb)
_L.curproc:	.long	_C_LABEL(curproc)
_L.cpl:		.long	_C_LABEL(cpl)
_L.proc0:	.long	_C_LABEL(proc0)
REG_SYMBOL(TTB)
FUNC_SYMBOL(tlb_invalidate_all)

/*
 * void savectx(struct pcb *pcb);
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
	rts
	 mov.l	@r15+, r14

/*
 * void ast(struct trapframe *frame):
 *	Check AST on exit from kernel to user mode.	
 */	
NENTRY(ast)
	mov.l	r8,	@-r15	
	sts.l	pr,	@-r15
	mov	r4,	r8
1:
	__INTR_MASK(r0, r1)
	mov.l	_L.astpending, r0
	mov.l	@r0,	r0
	tst	r0,	r0
	bt	2f
	/* If trap occurred in kernel , skip AST proc */
	mov.l	@(TF_SSR, r8), r1
	mov	#0x40,	r0
	swap.b	r0,	r0	
	swap.w	r0,	r0	/* r0 = 0x40000000 */
	and	r1,	r0	
	tst	r0,	r0	/* if (SSR.MD == 0) T = 1 */
	bf/s	2f
	 xor	r0,	r0
	mov.l	_L.astpending, r1
	mov.l	r0,	@r1		/* clear astpending */
	__INTR_UNMASK(r0, r1)
	mov.l	_L.T_ASTFLT, r1
	mov.l	r1, @(TF_TRAPNO, r8)	/* trapframe->tf_trapno = T_ASTFLT */
	mov.l	_L.trap, r0
	jsr	@r0
	 mov	r8,	r4
	bra	1b
	 nop
2:	
	lds.l	@r15+,	pr
	rts
	 mov.l	@r15+,	r8
	
	.align	2
_L.trap:	.long	_C_LABEL(trap)	
_L.astpending:	.long	_C_LABEL(astpending)
_L.T_ASTFLT:	.long	T_ASTFLT
