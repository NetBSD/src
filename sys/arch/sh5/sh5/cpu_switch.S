/*	$NetBSD: cpu_switch.S,v 1.15 2003/01/19 19:49:52 scw Exp $	*/

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
 * On entry, r1 must be the value of USR for the LWP
 */
#define	SAVE_CTX(pcb,mdf)						       \
	gettr	tr5, r0				/* Save target regs */	      ;\
	st.q	pcb, PCB_CTX_REGS_TR5, r0				      ;\
	gettr	tr6, r0							      ;\
	st.q	pcb, PCB_CTX_REGS_TR6, r0				      ;\
	gettr	tr7, r0							      ;\
	st.q	pcb, PCB_CTX_REGS_TR7, r0				      ;\
	getcon	sr, r0				/* Fetch status register */   ;\
	st.q	pcb, PCB_CTX_PC, r18		/* Save return address */     ;\
	st.q	pcb, PCB_CTX_SP, r15		/* Save stack pointer */      ;\
	st.q	pcb, PCB_CTX_FP, r14		/* Save frame pointer */      ;\
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
	andi	mdf, MDP_FPSAVED, r0	/* Skip FP save if already done */    ;\
	bne/u	r0, r63, tr0						      ;\
	shlri	r1, 8, mdf						      ;\
	andi	mdf, 0xff, mdf						      ;\
	cmpgt	mdf, r63, mdf		/* mdf = MDP_FPUSED if fpregs dirty */;\
	ori	mdf, MDP_FPSAVED, mdf					      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)			      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)			      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)		      ;\
	FPSV(pcb,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)		      ;\
	fmov.dq	dr0, r0			/* Save dr0 temporarily */	      ;\
	fgetscr	fr0			/* Always save FPSCR */		      ;\
	fst.s	pcb, PCB_CTX_FPREGS_FPSCR, fr0				      ;\
	fmov.qd	r0, dr0			/* Restore dr0 */		      ;\
99:


/******************************************************************************
 *
 * This is the opposite of the above macro.
 *
 * Uses: r0, r1 and tr0
 *
 * On entry, r1 must be the value of USR for the LWP
 * Note: Does not restore the Status Register
 */
#define	RESTORE_CTX(pcb,mdf)						       \
	ld.q	pcb, PCB_CTX_REGS_TR5, r0				      ;\
	ptabs/u	r0, tr5				/* Restore target regs */     ;\
	ld.q	pcb, PCB_CTX_REGS_TR6, r0				      ;\
	ptabs/u	r0, tr6							      ;\
	ld.q	pcb, PCB_CTX_REGS_TR7, r0				      ;\
	ptabs/u	r0, tr7							      ;\
	ld.q	pcb, PCB_CTX_PC, r18		/* Restore return address */  ;\
	ld.q	pcb, PCB_CTX_SP, r15		/* Restore stack pointer */   ;\
	ld.q	pcb, PCB_CTX_FP, r14		/* Restore frame pointer */   ;\
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
	fmov.dq	dr0, r0			/* Save dr0 temporarily */	      ;\
	fld.s	pcb, PCB_CTX_FPREGS_FPSCR, fr0				      ;\
	fputscr	fr0			/* Always restore the FPSCR */	      ;\
	fmov.qd	r0, dr0			/* Restore dr0 */		      ;\
	andi	mdf, MDP_FPSAVED, r0	/* Skip if FP state wasn't saved */   ;\
	beq/u	r0, r63, tr0						      ;\
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
 * When no LWPs are ready to run, cpu_switch branches to idle to
 * wait for someone to come ready.
 *
 * On entry, r29 points to "sched_whichqs".
 * On exit, r0 must hold the contents of "sched_whichqs"
 *
 * We don't need to preserve caller-save registers.
 */
ASENTRY_NOPROFILE(idle)
	getcon	kcr0, r0
	STPTR	r0, CI_CURLWP, r63	/* When idle, there is no curlwp */
	pta/u	1f, tr0

1:
#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_unlock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
#endif
	movi	SH5_CONREG_SR_IMASK_IPL0, r0
	putcon	r0, sr			/* Enable interrupts */

	synco				/* Sync the operand pipeline */
	synci				/* Sync the instruction pipeline */
	sleep				/* Snooze until an interrupt */
	synci				/* Sync the instruction pipeline */

	movi	SH5_CONREG_SR_IMASK_ALL, r0
	putcon	r0, sr			/* Disable interrupts */

#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_lock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
	pta/u	1b, tr0
#endif

	ld.l	r29, 0, r0		/* Fetch sched_whichqs */
	beq/u	r0, r63, tr0		/* Back around if nobody's awake */
	pta/l	Lsw2, tr0
	blink	tr0, r63		/* Otherwise, back to cpu_switch */


/******************************************************************************
 *
 * int cpu_switch(struct lwp *l)
 *
 * Find the highest priority LWP and resume it.
 *
 *  Argument 'l' is the current LWP
 *
 *  Returns 0 if no context switch was done, otherwise 1 if we
 *  switched to another context.
 */
ENTRY_NOPROFILE(cpu_switch)
#ifdef DEBUG
	pta/l	1f, tr0
	getcon	sr, r0
	LDC32(SH5_CONREG_SR_BL, r1)
	and	r0, r1, r0
	beq	r0, r63, tr0
	LEAF(_C_LABEL(panic), r0)
	ptabs/l	r0, tr0
	LEA(Lbadsw2_panic, r2)
	blink	tr0, r63
1:
#endif
	getcon	kcr0, r0
	LDPTR	r0, CI_CURPCB, r4	/* Fetch curpcb */
	ld.l	r2, L_MD_FLAGS, r3	/* Fetch md_flags */
	LDPTR	r2, L_MD_REGS, r0	/* Fetch pointer to trapframe */
	ld.q	r0, TF_USR, r1		/* Fetch saved USR */
	SAVE_CTX(r4,r3)			/* Save LWP context */
	st.l	r2, L_MD_FLAGS, r3	/* Save FP state */

	/*
	 * Save the old lwp in r30 for later, so we can check if we did
	 * in fact context switch away from it.
	 */
	add	r2, r63, r30

Lsw1:
	/*
	 * switch_exit() jumps here
	 */
	pta/u	_ASM_LABEL(idle), tr0
	LEA(_C_LABEL(sched_whichqs), r29)
	ld.l	r29, 0, r0		/* Fetch sched_whichqs */
	beq/u	r0, r63, tr0		/* Go to sleep if nobody's around */

Lsw2:
	/*
	 * idle() jumps here when there's work to do.
	 *
	 * Interrupts are blocked, sched_lock is held, and r0 contains
	 * the contents of a non-zero sched_whichqs.
	 */
	addi	r0, -1, r1		/* Slightly modified ffs(r0) */
	andc	r0, r1, r1
	nsb	r1, r1
	addi.l	r1, -62, r1		/* We know r0 != 0, so this works */
	sub.l	r63, r1, r1		/* Result in r1 */
	movi	1, r2
	shlld	r2, r1, r2		/* Set appropriate bit in r2 */
	andc	r0, r2, r5		/* Assume queue is empty, in r5 */

	/* Convert bit number into sched_qs offset */
#ifdef _LP64
	shlli	r1, 4, r2		/* sizeof(struct prochd) == 16 */
#else
	shlli	r1, 3, r2		/* sizeof(struct prochd) == 8 */
#endif

	LEA(_C_LABEL(sched_qs), r3)	/* Base of run queues */
	add	r3, r2, r3		/* r3 -> correct run queue */

	pta/u	Lbadsw, tr0

	/*
	 * Remove LWP from head of run queue
	 */
	LDPTR	r3, PH_LINK, r31	/* r31 = victim lwp, l */
	beq/u	r31, r3, tr0		/* Queue was empty! Panic stations! */
	LDPTR	r31, L_FORW, r4		/* r4 = l->l_forw */
	STPTR	r3, PH_LINK, r4		/* queue->ph_link = l->l_forw */
	STPTR	r4, L_BACK, r3		/* l->l_forw->l_back = queue */
	STPTR	r31, L_BACK, r63	/* l->l_back = NULL */

	sub	r3, r4, r4		/* r4 == 0 if queue is now empty */
	cmveq	r4, r5, r0		/* Update sched_whichqs bitmap */
	st.l	r29, 0, r0

Lsw3:
	/*
	 * Release the scheduler lock now that we're done with the run queue.
	 */
#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_unlock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
#endif

	/*
	 * At this point:
	 *   r30 == lwp we switched from (NULL if there wasn't one).
	 *   r31 == lwp we're about to switch to.
	 */
	pta/u	1f, tr0
	beq/u	r30, r63, tr0		/* Jump if no previous curlwp */

	pta/u	2f, tr0
	beq/u	r30, r31, tr0		/* Jump if switching to same lwp */

	/* Deactivate the old LWP's pmap */
	LEAF(_C_LABEL(pmap_deactivate), r0)
	ptabs/l	r0, tr0
	or	r30, r63, r2		/* Get old lwp to r2 */
	blink	tr0, r18

1:	getcon	kcr0, r0
	LDPTR	r31, L_ADDR, r1		/* Point to new lwp's pcb */
	STPTR	r0, CI_CURLWP, r31	/* New lwp is now current */
	STPTR	r0, CI_CURPCB, r1	/* Activate pcb */

	/* Activate the new LPW's pmap */
	LEAF(_C_LABEL(pmap_activate), r0)
	ptabs/l	r0, tr0
	or	r31, r63, r2
	blink	tr0, r18

	/*
	 * Check for restartable atomic sequences (RAS)
	 */
	pta/l	2f, tr0
	LDPTR	r31, L_PROC, r2
	ld.l	r2, P_NRAS, r0
	beq/l	r0, r63, tr0		/* Skip if no RAS registered for proc */
	LEAF(_C_LABEL(ras_lookup), r0)
	ptabs/l	r0, tr0
	LDPTR	r31, L_MD_REGS, r29	/* Fetch pointer to trapframe */
	ld.q	r29, TF_SPC, r3		/* Fetch user-space program-counter */
#ifndef _LP64
	add.l	r3, r63, r3		/* Sign-extend for ILP32 ABI */
#endif
	blink	tr0, r18		/* r2 = ras_lookup(l->l_proc, spc) */
	pta/l	2f, tr0
#ifndef _LP64
	add.l	r2, r63, r2		/* Sign-extend result for ILP32 ABI */
#endif
	beqi/l	r2, -1, tr0		/* Jump if no need to restart */
	st.q	r29, TF_SPC, r2		/* Otherwise, reset the LWP's PC */

2:
	getcon	kcr0, r0
	LDPTR	r31, L_ADDR, r3		/* Fetch new LWP's pcb */
	LDPTR	r31, L_MD_REGS, r1	/* Fetch pointer to trapframe */
	ld.l	r31, L_MD_FLAGS, r4
	movi	LSONPROC, r2		/* Flag new lwp as running */
	st.b	r31, L_STAT, r2		/* l->l_stat = LSONPROC */
	STPTR	r0, CI_CURLWP, r31	/* New lwp is now current */
	STPTR	r0, CI_CURPCB, r3	/* Activate pcb */
	st.l	r0, CI_WANT_RESCHED, r63 /* We've rescheduled */
	ld.q	r3, PCB_CTX_SR, r5	/* Fetch saved status register */
	ld.q	r1, TF_USR, r1		/* Fetch saved USR */
	cmpeq	r30, r31, r2		/* Compute return value */
	xori	r2, 1, r2		/* Correct for 'cmpeq' result */
	RESTORE_CTX(r3,r4)		/* Switch context to the new LWP */

#ifdef DEBUG
	pta/l	5f, tr0
	LDC32(SH5_CONREG_SR_BL, r1)
	and	r5, r1, r1
	beq/l	r1, r63, tr0
	LEAF(_C_LABEL(panic), r0)
	ptabs/l	r0, tr0
	LEA(Lbadsw3_panic, r2)
	blink	tr0, r63
5:
#endif

	ptabs/l	r18, tr0
	putcon	r5, sr			/* Restore interrupt mask */
	blink	tr0, r63		/* Done */

Lbadsw:
	LEAF(_C_LABEL(panic), r0)
	ptabs/l	r0, tr0
	LEA(Lbadsw_panic, r2)
	blink	tr0, r63

	.data
Lbadsw_panic:
	.asciz	"cpu_switch: bad switch!"
Lbadsw2_panic:
	.asciz	"cpu_switch: switch with SR.BL set in outgoing context!"
Lbadsw3_panic:
	.asciz	"cpu_switch: switch with SR.BL set in incoming context!"


/******************************************************************************
 *
 * void cpu_switchto(struct lwp *current, struct lwp *next)
 *
 * Switch to a specific next LWP.
 */
ENTRY(cpu_switchto)

	LDPTR	r2, CI_CURPCB, r4	/* Fetch current->l_addr */
	ld.l	r2, L_MD_FLAGS, r5	/* Fetch current->l_md.md_flags */
	LDPTR	r2, L_MD_REGS, r0	/* Fetch current->l_md.md_regs */
	ld.q	r0, TF_USR, r1		/* Fetch saved USR */
	SAVE_CTX(r4,r5)			/* Save 'current' context */
	st.l	r2, L_MD_FLAGS, r5	/* Update md_flags */
	pta/l	Lsw3, tr0
	or	r2, r63, r30
	or	r3, r63, r31
	blink	tr0, r63		/* Switch to 'next' */


/******************************************************************************
 *
 * void switch_exit(struct lwp *l, void (*exit_func)(struct lwp *))
 *
 * The LWP `l' is about to die.
 * Arrange for its obituary before jumping to cpu_switch() to find
 * another LWP to run.
 */
ENTRY(switch_exit)
	LEA(_C_LABEL(lwp0), r1)
	getcon	kcr0, r0
	ptabs/l	r3, tr0
	LDPTR	r1, L_ADDR, r1
	STPTR	r0, CI_CURPCB, r1	/* Set something useful in CURPCB */
	STPTR	r0, CI_CURLWP, r63	/* No LWP is current */
	ld.q	r1, PCB_CTX_SP, r15	/* Switch to lwp0's stack */
	ld.q	r1, PCB_CTX_FP, r14

	blink	tr0, r18		/* (*exit_func)(l) */

#ifdef LOCKDEBUG
	LEAF(_C_LABEL(sched_lock_idle), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18
#endif

	pta/l	Lsw1, tr0
	add	r63, r63, r30		/* Previous LWP is NULL in cpu_switch */
	blink	tr0, r63


/******************************************************************************
 *
 * int sh5_savectx(u_int usr, int mdf, struct pcb *pcb)
 *
 * Save state in the PCB
 */
ENTRY(sh5_savectx)
	ptabs/l	r18, tr1
	or	r2, r63, r1		/* Get USR to r1 */
	SAVE_CTX(r4, r3)
	or	r3, r63, r2
	blink	tr1, r63



/******************************************************************************
 *
 * int sh5_fpsave(u_int usr, struct pcb *pcb)
 *
 * Save FP state in the PCB
 *
 * Returns a value suitable for storing in curlwp->l_md.md_flags to
 * indicate if the FPU state was used/saved.
 */
ENTRY(sh5_fpsave)
	ptabs/l	r18, tr1
	getcon	sr, r0
	or	r2, r63, r1
	movi	0, r2
	shlri	r0, SH5_CONREG_SR_FD_SHIFT, r0
	andi	r0, 1, r0
	bne/u	r0, r63, tr1		/* Skip FP save if FPU disabled */
	shlri	r1, 8, r0
	andi	r0, 0xff, r0
	cmpgt	r0, r63, r2		/* r2 = MDP_FPUSED if fpregs dirty */
	ori	r2, MDP_FPSAVED, r2
	FPSV(r3,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)
	FPSV(r3,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)
	FPSV(r3,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)
	FPSV(r3,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)
	FPSV(r3,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)
	FPSV(r3,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)
	FPSV(r3,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)
	FPSV(r3,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)
	fmov.dq	dr0, r0			/* Save dr0 temporarily */
	fgetscr	fr0			/* Always save FPSCR */
	fst.s	r3, PCB_CTX_FPREGS_FPSCR, fr0
	fmov.qd	r0, dr0			/* Restore dr0 */
	blink	tr1, r63


/******************************************************************************
 *
 * void sh5_fprestore(u_int usr, struct pcb *pcb)
 *
 * Restore FP state from the PCB
 */
ENTRY(sh5_fprestore)
	ptabs/l	r18, tr1
	getcon	sr, r0
	or	r2, r63, r1
	shlri	r0, SH5_CONREG_SR_FD_SHIFT, r0
	andi	r0, 1, r0
	bne/u	r0, r63, tr1		/* Skip FP save if FPU disabled */
	fmov.dq	dr0, r0			/* Save dr0 temporarily */
	fld.s	r3, PCB_CTX_FPREGS_FPSCR, fr0
	fputscr	fr0			/* Always restore FPSCR */
	fmov.qd	r0, dr0			/* Restore dr0 */
	FPRS(r3,PCB_CTX_FPREGS_DR0,dr0,dr2,dr4,dr6)
	FPRS(r3,PCB_CTX_FPREGS_DR8,dr8,dr10,dr12,dr14)
	FPRS(r3,PCB_CTX_FPREGS_DR16,dr16,dr18,dr20,dr22)
	FPRS(r3,PCB_CTX_FPREGS_DR24,dr24,dr26,dr28,dr30)
	FPRS(r3,PCB_CTX_FPREGS_DR32,dr32,dr34,dr36,dr38)
	FPRS(r3,PCB_CTX_FPREGS_DR40,dr40,dr42,dr44,dr46)
	FPRS(r3,PCB_CTX_FPREGS_DR48,dr48,dr50,dr52,dr54)
	FPRS(r3,PCB_CTX_FPREGS_DR56,dr56,dr58,dr60,dr62)
	blink	tr1, r63
