/*	$NetBSD: cpu_switch.S,v 1.1 2002/07/05 13:32:04 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ##########
 *
 * This is not a standalone file.
 * To use it, put #include <sh5/sh5/cpu_switch.S> in your port's locore.S
 *
 * ##########
 */

	.section .text..SHmedia32,"ax"
	.align 3


/*
 * Helper macro for SAVE_CTX which skips 8 FP register saves if the USR
 * register (right-shifted in r1) indicates the FP group is unused.
 */
#define	FPSV(pcb,off,d1,d2,d3,d4)					       \
	pta/l	$+32, tr0						      ;\
	andi	r1, 256, r0						      ;\
	shlri	r1, 1, r1						      ;\
	beq/l	r0, r63, tr0						      ;\
	fst.d	pcb, off+0, d1						      ;\
	fst.d	pcb, off+8, d2						      ;\
	fst.d	pcb, off+16, d3						      ;\
	fst.d	pcb, off+24, d4

/*
 * Helper macro for RESTORE_CTX which skips restoring 8 FP register if the
 * USR register (right-shifted in r1) indicates the FP group was unused.
 */
#define	FPRS(pcb,off,d1,d2,d3,d4)					       \
	pta/l	$+32, tr0						      ;\
	andi	r1, 256, r0						      ;\
	shlri	r1, 1, r1						      ;\
	beq/l	r0, r63, tr0						      ;\
	fld.d	pcb, off+0, d1						      ;\
	fld.d	pcb, off+8, d2						      ;\
	fld.d	pcb, off+16, d3						      ;\
	fld.d	pcb, off+24, d4


/******************************************************************************
 *
 * This macro saves the current context into the pcb.
 *
 * Uses: r0, r1 and tr0
 *
 * On entry, r1 must be the value of USR for the process
 */
#define	SAVE_CTX(pcb)							       \
	gettr	tr5, r0				/* Save target regs */	      ;\
	st.q	pcb, PCB_CTX_REGS_TR5, r0				      ;\
	gettr	tr6, r0							      ;\
	st.q	pcb, PCB_CTX_REGS_TR6, r0				      ;\
	gettr	tr7, r0							      ;\
	st.q	pcb, PCB_CTX_REGS_TR7, r0				      ;\
	getcon	sr, r0				/* Fetch status register */   ;\
	st.q	pcb, PCB_CTX_PC, r18		/* Save return address */     ;\
	st.q	pcb, PCB_CTX_SP, r15		/* Save stack pointer */      ;\
	st.q	pcb, PCB_CTX_SR, r0		/* Save status register */    ;\
	st.q	pcb, PCB_CTX_REGS_R10, r10	/* Save callee-saved regs */  ;\
	st.q	pcb, PCB_CTX_REGS_R11, r11				      ;\
	st.q	pcb, PCB_CTX_REGS_R12, r12				      ;\
	st.q	pcb, PCB_CTX_REGS_R13, r13				      ;\
	st.q	pcb, PCB_CTX_REGS_R28, r28				      ;\
	st.q	pcb, PCB_CTX_REGS_R29, r29				      ;\
	st.q	pcb, PCB_CTX_REGS_R30, r30				      ;\
	st.q	pcb, PCB_CTX_REGS_R31, r31				      ;\
	st.q	pcb, PCB_CTX_REGS_R32, r32				      ;\
	st.q	pcb, PCB_CTX_REGS_R33, r33				      ;\
	st.q	pcb, PCB_CTX_REGS_R34, r34				      ;\
	st.q	pcb, PCB_CTX_REGS_R35, r35				      ;\
	st.q	pcb, PCB_CTX_REGS_R44, r44				      ;\
	st.q	pcb, PCB_CTX_REGS_R45, r45				      ;\
	st.q	pcb, PCB_CTX_REGS_R46, r46				      ;\
	st.q	pcb, PCB_CTX_REGS_R47, r47				      ;\
	st.q	pcb, PCB_CTX_REGS_R48, r48				      ;\
	st.q	pcb, PCB_CTX_REGS_R49, r49				      ;\
	st.q	pcb, PCB_CTX_REGS_R50, r50				      ;\
	st.q	pcb, PCB_CTX_REGS_R51, r51				      ;\
	st.q	pcb, PCB_CTX_REGS_R52, r52				      ;\
	st.q	pcb, PCB_CTX_REGS_R53, r53				      ;\
	st.q	pcb, PCB_CTX_REGS_R54, r54				      ;\
	st.q	pcb, PCB_CTX_REGS_R55, r55				      ;\
	st.q	pcb, PCB_CTX_REGS_R56, r56				      ;\
	st.q	pcb, PCB_CTX_REGS_R57, r57				      ;\
	st.q	pcb, PCB_CTX_REGS_R58, r58				      ;\
	st.q	pcb, PCB_CTX_REGS_R59, r59				      ;\
	pta/u	99f, tr0			/* Assume no FP regs to save*/;\
	shlri	r0, SH5_CONREG_SR_FD_SHIFT, r0				      ;\
	andi	r0, 1, r0						      ;\
	bne/u	r0, r63, tr0		/* Skip FP save if FPU disabled */    ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)			      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)			      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)		      ;\
	fgetscr	fr0							      ;\
	fst.d	pcb, PCB_CTX_FPREGS_FPSCR, dr0				      ;\
99:


/******************************************************************************
 *
 * This is the opposite of the above macro.
 *
 * Uses: r0, r1 and tr0
 *
 * On entry, r1 must be the value of USR for the process
 * Note: Does not restore the Status Register
 */
#define	RESTORE_CTX(pcb)						       \
	ld.q	pcb, PCB_CTX_REGS_TR5, r0				      ;\
	ptabs/u	r0, tr5				/* Restore target regs */     ;\
	ld.q	pcb, PCB_CTX_REGS_TR6, r0				      ;\
	ptabs/u	r0, tr6							      ;\
	ld.q	pcb, PCB_CTX_REGS_TR7, r0				      ;\
	ptabs/u	r0, tr7							      ;\
	ld.q	pcb, PCB_CTX_PC, r18		/* Restore return address */  ;\
	ld.q	pcb, PCB_CTX_SP, r15		/* Restore stack pointer */   ;\
	ld.q	pcb, PCB_CTX_REGS_R10, r10	/* Restore callee-saved regs*/;\
	ld.q	pcb, PCB_CTX_REGS_R11, r11				      ;\
	ld.q	pcb, PCB_CTX_REGS_R12, r12				      ;\
	ld.q	pcb, PCB_CTX_REGS_R13, r13				      ;\
	ld.q	pcb, PCB_CTX_REGS_R28, r28				      ;\
	ld.q	pcb, PCB_CTX_REGS_R29, r29				      ;\
	ld.q	pcb, PCB_CTX_REGS_R30, r30				      ;\
	ld.q	pcb, PCB_CTX_REGS_R31, r31				      ;\
	ld.q	pcb, PCB_CTX_REGS_R32, r32				      ;\
	ld.q	pcb, PCB_CTX_REGS_R33, r33				      ;\
	ld.q	pcb, PCB_CTX_REGS_R34, r34				      ;\
	ld.q	pcb, PCB_CTX_REGS_R35, r35				      ;\
	ld.q	pcb, PCB_CTX_REGS_R44, r44				      ;\
	ld.q	pcb, PCB_CTX_REGS_R45, r45				      ;\
	ld.q	pcb, PCB_CTX_REGS_R46, r46				      ;\
	ld.q	pcb, PCB_CTX_REGS_R47, r47				      ;\
	ld.q	pcb, PCB_CTX_REGS_R48, r48				      ;\
	ld.q	pcb, PCB_CTX_REGS_R49, r49				      ;\
	ld.q	pcb, PCB_CTX_REGS_R50, r50				      ;\
	ld.q	pcb, PCB_CTX_REGS_R51, r51				      ;\
	ld.q	pcb, PCB_CTX_REGS_R52, r52				      ;\
	ld.q	pcb, PCB_CTX_REGS_R53, r53				      ;\
	ld.q	pcb, PCB_CTX_REGS_R54, r54				      ;\
	ld.q	pcb, PCB_CTX_REGS_R55, r55				      ;\
	ld.q	pcb, PCB_CTX_REGS_R56, r56				      ;\
	ld.q	pcb, PCB_CTX_REGS_R57, r57				      ;\
	ld.q	pcb, PCB_CTX_REGS_R58, r58				      ;\
	ld.q	pcb, PCB_CTX_REGS_R59, r59				      ;\
	ld.q	pcb, PCB_CTX_SR, r0	/* Fetch status register */	      ;\
	pta/u	99f, tr0		/* Assume no FP regs to restore */    ;\
	shlri	r0, SH5_CONREG_SR_FD_SHIFT, r0				      ;\
	andi	r0, 1, r0						      ;\
	bne/u	r0, r63, tr0		/* Skip FP restore if FPU disabled */ ;\
	fld.d	pcb, PCB_CTX_FPREGS_FPSCR, dr0				      ;\
	fputscr	fr0							      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)			      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)			      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)		      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)		      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)		      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)		      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)		      ;\
	FPRS(pcb,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)		      ;\
99:


/******************************************************************************
 *
 * When no processes are ready to run, cpu_switch branches to idle to
 * wait for someone to come ready.
 *
 * On entry, r11 points to "sched_whichqs".
 * On exit, r0 must hold the contents of "sched_whichqs"
 *
 * We don't need to preserve caller-save registers.
 */
ENTRY(idle)
	/* Deactivate the old process' pmap */
	getcon	kcr0, r0
	LDPTR	r0, CI_CURPROC, r2
	pta/u	1f, tr0
	beq/u	r2, r63, tr0		/* curproc is already NULL! */
	LEAF(_C_LABEL(pmap_deactivate), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
	getcon	kcr0, r0
	STPTR	r0, CI_CURPROC, r63	/* When idle, there is no curproc */
	pta/u	1f, tr0

1:
#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_unlock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
#endif
	movi	SH5_CONREG_SR_IMASK_IPL0, r0
	putcon	r0, sr			/* Enable interrupts */

#ifndef SIMULATED_CLOCK
	synci				/* Sync the instruction pipeline */
	synco				/* Sync the operand pipeline */
	sleep				/* Snooze until an interrupt */
	synci				/* Sync the instruction pipeline */
#else
	LEA(_C_LABEL(_sh5_simclock_ticks), r1)
	ld.l	r1, 0, r2
	addz.l	r2, r63, r2
	pta/l	Lno_simclockint, tr0
	getcon	ctc, r0
	bgt/l	r2, r0, tr0
	LDC32(64000, r2)
	add	r0, r2, r0
	st.l	r1, 0, r0
	trapa	r63			/* "Magic" simulated clock trap */
Lno_simclockint:
#endif
	movi	SH5_CONREG_SR_IMASK_ALL, r0
	putcon	r0, sr			/* Disable interrupts */

#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_lock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
	pta/u	1b, tr0
#endif

	ld.l	r11, 0, r0		/* Fetch sched_whichqs */
	beq/u	r0, r63, tr0		/* Back around if nobody's awake */
	pta/l	Lsw2, tr0
	blink	tr0, r63		/* Otherwise, back to cpu_switch */


/******************************************************************************
 *
 * void cpu_switch(void)
 *
 * Find the highest priority process and resume it.
 *
 * XXX: Still needs to be optimised to avoid saving/restoring state if
 *      resuming curproc.
 */
ENTRY(cpu_switch)
	getcon	kcr0, r0
	LDPTR	r0, CI_CURPCB, r2	/* Fetch curpcb */
	LDPTR	r0, CI_CURPROC, r0	/* Fetch curproc */
	LDPTR	r0, P_MD_REGS, r0	/* Fetch pointer to trapframe */
	ld.q	r0, TF_USR, r1		/* Fetch saved USR */
	SAVE_CTX(r2)			/* Save process context */

Lsw1:
	pta/u	_C_LABEL(idle), tr0
	LEA(_C_LABEL(sched_whichqs), r11)
	ld.l	r11, 0, r0		/* Fetch sched_whichqs */
	beq/u	r0, r63, tr0		/* Go to sleep if nobody's around */

Lsw2:
	/*
	 * Interrupts are blocked, sched_lock is held, and r0 contains
	 * the contents of a non-zero sched_whichqs.
	 */
	addz.l	r0, r63, r0		/* Correct for sign extension */
	nsb	r0, r1			/* Count sign bits in r0 */
	addi	r1, -31, r1		/* Correct for 64-bitness of "nsb" */
	xori	r1, 31, r1		/* Convert to range 31-0 */
	movi	1, r2
	shlld	r2, r1, r2		/* Set appropriate bit in r2 */
	andc	r0, r2, r5		/* Assume queue is empty, in r5 */

	/* Convert bit number into sched_qs offset */
#ifdef _ILP64
	shlli	r1, 4, r2		/* sizeof(struct prochd) == 16 */
#else
	shlli	r1, 3, r2		/* sizeof(struct prochd) == 8 */
#endif

	LEA(_C_LABEL(sched_qs), r3)	/* Base of run queues */
	add	r3, r2, r3		/* r3 -> correct run queue */

	pta/u	Lbadsw, tr0

	/*
	 * Remove process from head of run queue
	 */
	LDPTR	r3, PH_LINK, r2		/* r2 = victim process, p */
	beq/u	r2, r63, tr0		/* Queue was empty! Panic stations! */
	LDPTR	r2, P_FORW, r4		/* r4 = p->p_forw */
	STPTR	r3, PH_LINK, r4		/* queue->ph_link = p->p_forw */
	STPTR	r4, P_BACK, r3		/* p->p_forw->p_back = queue */
	STPTR	r2, P_BACK, r63		/* p->p_back = NULL */

	sub	r3, r4, r4		/* r4 == 0 if queue is now empty */
	cmveq	r4, r5, r0		/* Update sched_whichqs bitmap */
	st.l	r11, 0, r0

	/*
	 * At this point: r2 == process we're about to switch to
	 */
	getcon	kcr0, r10
	LDPTR	r10, CI_CURPROC, r10	/* Fetch curproc (NULL if we idled) */
	pta/u	1f, tr0
	beq/u	r10, r63, tr0		/* Jump if no previous curproc */

	pta/u	2f, tr0
	beq/u	r2, r10, tr0		/* Jump if switching to same process */

	/* Deactivate the old process' pmap */
	LEAF(_C_LABEL(pmap_deactivate), r0)
	ptabs/l	r0, tr0
	or	r2, r63, r11		/* Save new proc in r11 */
	or	r10, r63, r2		/* Get old proc to r2 */
	blink	tr0, r18
	or	r11, r63, r2		/* Restore r2 */

1:	getcon	kcr0, r0
	LDPTR	r2, P_ADDR, r1		/* Point to process' pcb */
	STPTR	r0, CI_CURPROC, r2	/* New process is now current */
	STPTR	r0, CI_CURPCB, r1	/* Activate pcb */

	/* Activate the new process' pmap */
	LEAF(_C_LABEL(pmap_activate), r0)
	ptabs/l	r0, tr0
	or	r2, r63, r10		/* Save new proc in r10 */
	blink	tr0, r18
	LDPTR	r10, P_ADDR, r3		/* Fetch new process' pcb */
	or	r10, r63, r2		/* new proc to r2 */

2:
	LDPTR	r2, P_MD_REGS, r1	/* Fetch pointer to trapframe */
	ld.q	r1, TF_USR, r1		/* Fetch saved USR */
	RESTORE_CTX(r3)			/* Switch context to the new process */

	/*
	 * We're running in the new process context. Only caller-saved
	 * registers are available now.
	 */
#ifdef LOCKDEBUG
	addi	r15, -8, r15
	LEAF(_C_LABEL(sched_unlock_idle), r0)
	ptabs/l	r0, tr0
	st.q	r15, 0, r18
	blink	tr0, r18
	ld.q	r15, 0, r18
	addi	r15, 8, r15
#endif
	getcon	kcr0, r0
	LDPTR	r0, CI_CURPROC, r1	/* Fetch curproc */
	movi	SONPROC, r2		/* Flag process as running */
	st.b	r1, P_STAT, r2
	st.l	r1, CI_WANT_RESCHED, r63 /* We've rescheduled */

	LDPTR	r0, CI_CURPCB, r0	/* Fetch curpcb */
	ld.q	r0, PCB_CTX_SR, r0	/* Fetch saved status register */
	putcon	r0, sr			/* Restore interrupt mask */

	ptabs/l	r18, tr0
	movi	1, r2			/* Non-zero return value */
	blink	tr0, r63		/* Done */

Lbadsw:
	LEAF(_C_LABEL(panic), r0)
	ptabs/l	r0, tr0
	LEA(Lbadsw_panic, r2)
	blink	tr0, r63

	.data
Lbadsw_panic:
	.asciz	"cpu_switch: bad switch!"


/******************************************************************************
 *
 * void switch_exit(struct proc *p)
 *
 * The process `p' is about to die.
 * Arrange for its obituary before jumping to cpu_switch() to find
 * another process to run.
 */
ENTRY(switch_exit)
	getcon	kcr0, r0
	LEA(Lnullpcb,r1)
	movi	USPACE-8, r3
	STPTR	r0, CI_CURPCB, r1	/* Set something useful in CURPCB */
	add	r1, r3, r15		/* Switch to a temporary stack */
					/* XXX: Should be per cpu... */
	LEAF(_C_LABEL(exit2), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18

	getcon	kcr0, r0
	STPTR	r0, CI_CURPROC, r63	/* No process is current */

#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_lock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
#endif

	pta/l	Lsw1, tr0
	blink	tr0, r63

	.comm	Lnullpcb,USPACE,0



/******************************************************************************
 *
 * int sh5_fpsave(u_int usr, struct pcb *pcb)
 *
 * Save FP state in the PCB
 */
ENTRY(sh5_fpsave)
	ptabs/l	r18, tr0
	getcon	sr, r0
	or	r2, r63, r1
	shlri	r0, SH5_CONREG_SR_FD_SHIFT, r0
	andi	r0, 1, r0
	bne/u	r0, r63, tr0		/* Skip FP save if FPU disabled */
	FPSV(r3,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)
	FPSV(r3,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)
	FPSV(r3,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)
	FPSV(r3,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)
	FPSV(r3,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)
	FPSV(r3,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)
	FPSV(r3,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)
	FPSV(r3,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)
	fgetscr	fr0
	fst.d	r3, PCB_CTX_FPREGS_FPSCR, dr0
	blink	tr0, r63


/******************************************************************************
 *
 * int sh5_fprestore(u_int usr, struct pcb *pcb)
 *
 * Restore FP state from the PCB
 */
ENTRY(sh5_fprestore)
	ptabs/l	r18, tr0
	getcon	sr, r0
	or	r2, r63, r1
	shlri	r0, SH5_CONREG_SR_FD_SHIFT, r0
	andi	r0, 1, r0
	bne/u	r0, r63, tr0		/* Skip FP save if FPU disabled */
	FPRS(r3,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)
	FPRS(r3,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)
	FPRS(r3,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)
	FPRS(r3,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)
	FPRS(r3,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)
	FPRS(r3,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)
	FPRS(r3,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)
	FPRS(r3,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)
	fgetscr	fr0
	fst.d	r3, PCB_CTX_FPREGS_FPSCR, dr0
	blink	tr0, r63
