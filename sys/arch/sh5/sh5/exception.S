/*	$NetBSD: exception.S,v 1.20 2003/01/19 19:49:53 scw Exp $	*/

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
 * To use it, put #include <sh5/sh5/exception.S> in your port's locore.S
 *
 * ##########
 */

#include "opt_ddb.h"

/*
 * SH5 Exception Handling 101
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Or, "How To Make Life Hell for an Operating System".
 *
 * The _EXCEPTION_ENTRY macro is used at the start of most exception
 * handlers.
 *
 * It saves some critical machine state in the scratch area and ensures
 * we're running on the kernel stack.
 *
 * A stack frame is created on the kernel stack and the saved SR and PC,
 * together with the critical data from the scratch area are stashed on it.
 *
 * The macro parameter, "sz", specifies the size of stack frame to
 * create. This will either be SZ_TRAPFRAME or SZ_INTRFRAME.
 *
 * At the end of it all, the following registers are available for use:
 *
 *   r0, r1, r2, r15 and tr0.
 *
 * Note that r1 holds the value of SR on entry to the exception and should
 * be written back to SR as soon as enough data contingent with the exception
 * has been saved by the handler. After _EXCEPTION_ENTRY, r2 points to the
 * cpu_info for this cpu so that handlers can retrieve exception contingent
 * data, amongst other stuff.
 *
 * An explanation of why this code has to Tread Carefully is appropos...
 *
 * Basically, we need to save the volatile machine state on a stack.
 * This will be the kernel stack for the currently executing process.
 * To do this requires that we free up some registers by saving them in
 * a scratch area.
 *
 * While neither the saved values in the scratch area nor the exception
 * contingent data (e.g. EXPEVT, INTEVT, etc) are saved on the stack, we
 * are considered to be inside the Critical Section (see below).
 *
 * Saving state is complicated some more by the fact that we Really Want
 * to preserve the current User Status register (USR). This is tricky since
 * any write to a register, even in kernel mode, will change USR. We get
 * around this using a combination of r24 and kcr1.
 *
 * Before we can save state, we have to make sure we're running on the
 * right stack. If the exception came from kernel-mode, we're already
 * running on a kernel stack so we don't need to do the switch.
 *
 * If the exception came from user-mode, we have to switch from the
 * process' user-mode stack onto the top of the kernel stack in the
 * process' USPACE (we maintain a pointer to cpu_info in kcr0, so it's
 * easy to find the kernel stack from there).
 *
 * So far so good.
 *
 * The problem is that USPACE could be allocated from the kernel's KSEG1 VA
 * space. This means that we could take a DTLB miss exception while trying
 * to save state to it inside the critical section! However, further
 * exceptions are currently blocked by the status register's SR.BL bit being
 * set, so any DTLB miss exception would never happen and all hell would
 * break loose.
 *
 * So, we clear SR.BL to allow DTLB miss exceptions, safe in the knowledge
 * that no other *synchronous* exceptions can happen in the critical section.
 *
 * However, we could still take an asynchronous hardware interrupt, so to
 * guard against those we must set SR.IMASK to 0xf.
 *
 * Note that the TLB miss handler also makes use of the OS-reserved r24/kcr1
 * so if a TLB miss occurs while we're saving state on the kernel stack,
 * r24/kcr1 will be trashed. Therefore, we must stop using them before
 * touching the stack.
 *
 * The only remaining problem is NMIs. There's nothing we can do to mask
 * those so taking one inside the critical section will lead to all sorts
 * of bogosity. We can make this work by stating that we cannot resume
 * normal kernel operation in the event of an NMI. Basically, we're toast.
 *
 * Note: We're not worried about Debug Interrupts, as we don't deal with
 * them anyway.
 *
 * And there you have it. Exceptions from hell.
 */

#ifdef PORTMASTER
#define	CHECK_CRITICAL(base)						       \
	pta/l	50f, tr0		/* Assume critical section is free */ ;\
	ld.q	base, ES(ES_CRITICAL), r0 /* Fetch owner */		      ;\
	beq/l	r0, r63, tr0		/* Jump if it's free */		      ;\
	pta/l	Lsh5_critical_fault, tr0				      ;\
	blink	tr0, r0			/* Otherwise, we're toast. */	      ;\
50:
#else
#define	CHECK_CRITICAL(base)	/* */
#endif

/*
 * The following three macros allow the _EXCEPTION_*, _INTR_FRAME_*
 * and _TRAP_FRAME_* macros to work with the two different exception
 * frame sizes.
 */
#define	SFO(r,sz)	((r) + ((sz)-SZ_INTRFRAME))
#define	IFO(r,sz)	((r) + ((sz)-SZ_INTRFRAME))
#define	TFO(r,sz)	(r)
#define	ES(o)		((o)+CI_ESCRATCH)

/*
 * --->>>> README <<<<---
 * The number of instructions in this has become fairly critical.
 * DO NOT add any more instructions without first checking there is space
 * for them in the exception handlers.
 */
#define	_EXCEPTION_ENTRY(sz,id)						       \
	getcon	usr, r24		/* Stash USR somewhere safe for now */;\
	putcon	r24, kcr1						      ;\
	getcon	kcr0, r24						      ;\
	st.q	r24, ES(ES_R0), r0	/* Save r0 and tr0 temporarily */     ;\
	gettr	tr0, r0							      ;\
	st.q	r24, ES(ES_TR0), r0					      ;\
	CHECK_CRITICAL(r24)						      ;\
	movi	id, r0			/* Identify who owns critical sect */ ;\
	st.q	r24, ES(ES_CRITICAL), r0				      ;\
	st.q	r24, ES(ES_R1), r1					      ;\
	st.q	r24, ES(ES_R2), r2					      ;\
	st.q	r24, ES(ES_R15), r15					      ;\
	getcon	expevt, r0		/* Save critical data */	      ;\
	st.q	r24, ES(ES_EXPEVT), r0					      ;\
	getcon	intevt, r0						      ;\
	st.q	r24, ES(ES_INTEVT), r0					      ;\
	getcon	tea, r0							      ;\
	st.q	r24, ES(ES_TEA), r0					      ;\
	getcon	tra, r0							      ;\
	st.q	r24, ES(ES_TRA), r0					      ;\
	getcon	kcr1, r0						      ;\
	st.q	r24, ES(ES_USR), r0					      ;\
	getcon	spc, r0							      ;\
	st.q	r24, ES(ES_SPC), r0					      ;\
	getcon	ssr, r0							      ;\
	st.q	r24, ES(ES_SSR), r0					      ;\
	LDPTR	r24, CI_CURPCB, r1	/* Get curpcb */		      ;\
	movi	USPACE, r2		/* Offset of top of kernel stack */   ;\
	add	r2, r1, r1						      ;\
	shlri	r0, SH5_CONREG_SR_MD_SHIFT, r0	/* Get SSR.MD to bit#0 */     ;\
	andi	r0, 1, r0		/* r0 == 0 if came from user mode */  ;\
	cmveq	r0, r1, r15		/* Switch stacks if necessary */      ;\
	movi	sz, r0							      ;\
	sub	r15, r0, r15		/* Make space for trapframe */	      ;\
	getcon	sr, r1			/* Fetch current status register */   ;\
	ori	r1, SH5_CONREG_SR_IMASK_ALL, r0				      ;\
	putcon	r0, sr							      ;\
	LDC32(SH5_CONREG_SR_BL, r2)	/* Unblock exceptions. This allows */ ;\
	andc	r0, r2, r0		/* us to take a TLB miss exception */ ;\
	andc	r1, r2, r1						      ;\
	putcon	r0, sr			/* Now safe to touch kernel stack */  ;\
	or	r24, r63, r2		/* Drop use of r24 now */	      ;\
	ld.q	r2, ES(ES_R0), r0					      ;\
	st.q	r15, IFO(IF_R0,sz), r0	/* Save original r0 */		      ;\
	ld.q	r2, ES(ES_SSR), r0					      ;\
	st.q	r15, SFO(SF_SSR,sz), r0	/* Save SSR */			      ;\
	ld.q	r2, ES(ES_SPC), r0					      ;\
	st.q	r15, SFO(SF_SPC,sz), r0	/* Save SPC */			      ;\
	ld.q	r2, ES(ES_USR), r0					      ;\
	st.q	r15, SFO(SF_USR,sz), r0	/* Save USR */			      ;\
	ld.q	r2, ES(ES_R1), r0					      ;\
	st.q	r15, IFO(IF_R1,sz), r0	/* Save original r1 */		      ;\
	ld.q	r2, ES(ES_R2), r0					      ;\
	st.q	r15, IFO(IF_R2,sz), r0	/* Save original r2 */		      ;\
	ld.q	r2, ES(ES_R15), r0					      ;\
	st.q	r15, IFO(IF_R15,sz), r0	/* Save original r15 */		      ;\
	ld.q	r2, ES(ES_TR0), r0					      ;\
	st.q	r15, IFO(IF_TR0,sz), r0	/* Save tr0 */

/*
 * Finish handling an exception and return to the previous context.
 * Only r0, r1, r15 and tr0 are available at this point.
 *
 * Note that we can't block synchronous exceptions here in case touching
 * the kernel stack causes a DLTB miss.
 */
#define	_EXCEPTION_EXIT(sz,id)						       \
	getcon	sr, r0							      ;\
	ori	r0, SH5_CONREG_SR_IMASK_ALL, r2 /* No IRQs please */	      ;\
	putcon	r2, sr							      ;\
	getcon	kcr0, r1						      ;\
	CHECK_CRITICAL(r1)						      ;\
	movi	(id) | CRIT_EXIT, r0					      ;\
	st.q	r1, ES(ES_CRITICAL), r0	/* Entering critical section */	      ;\
	ld.q	r15, SFO(SF_SSR,sz), r0					      ;\
	LDC32(SH5_CONREG_SR_ASID_MASK << SH5_CONREG_SR_ASID_SHIFT, r24)	      ;\
	and	r2, r24, r2		/* Get current ASID into r2 */	      ;\
	andc	r0, r24, r0		/* Clear ASID bits in saved SSR  */   ;\
	or	r2, r0, r0		/* Insert current ASID in saved SSR */;\
	st.q	r1, ES(ES_SSR), r0					      ;\
	ld.q	r15, SFO(SF_SPC,sz), r0					      ;\
	st.q	r1, ES(ES_SPC), r0					      ;\
	ld.q	r15, SFO(SF_USR,sz), r0					      ;\
	st.q	r1, ES(ES_USR), r0					      ;\
	ld.q	r15, IFO(IF_R0,sz), r0					      ;\
	st.q	r1, ES(ES_R0), r0					      ;\
	ld.q	r15, IFO(IF_TR0,sz), r0	/* Restore saved TR0 */		      ;\
	ptabs/u	r0, tr0							      ;\
	ld.q	r15, IFO(IF_R1,sz), r1					      ;\
	ld.q	r15, IFO(IF_R2,sz), r2					      ;\
	ld.q	r15, IFO(IF_R15,sz), r15/* Back on previous context's stack */;\
	getcon	sr, r0							      ;\
	LDC32(SH5_CONREG_SR_BL | SH5_CONREG_SR_IMASK_ALL, r24)		      ;\
	or	r0, r24, r0		/* We can now block synchronous */    ;\
	putcon	r0, sr			/* exceptions now that we're done */  ;\
	getcon	kcr0, r24		/* touching the kernel stack */	      ;\
	ld.q	r24, ES(ES_SPC), r0					      ;\
	putcon	r0, spc							      ;\
	ld.q	r24, ES(ES_SSR), r0					      ;\
	putcon	r0, ssr							      ;\
	ld.q	r24, ES(ES_R0), r0					      ;\
	st.q	r24, ES(ES_CRITICAL), r63 /* Leaving critical section */      ;\
	ld.q	r24, ES(ES_USR), r24					      ;\
	putcon	r24, usr						      ;\
	synco								      ;\
	rte								      ;\
	nop								      ;\
	nop								      ;\
	nop								      ;\
	nop								      ;\
	nop


/*
 * Save an interrupt frame on the stack (basically, caller-saved registers).
 *
 * The `sz' parameter specifies the size of the current stack frame.
 * This will be either SZ_INTRFRAME or SZ_TRAPFRAME.
 *
 * Note: Assumes r0, r1, r2 and tr0 were saved by _EXCEPTION_ENTRY macro.
 *
 * Note: We have to save r10-r14 here even though they're not true Caller-Saved
 * registers. Basically, C code *will* save the registers as part of a
 * function's prologue. However if the kernel is IPL32, only the low 32-bits
 * will be saved (yes, this is how the ABI specifies it should happen). This
 * will kind of screw things up should we be called from LP64 user code as
 * assignments in ILP32 will sign-extend to the upper 32-bits regardless.
 *
 * And anyway, we want to fix up r14 so kernel stack traces work.
 */
#define	_INTR_FRAME_SAVE(sz)						      \
	st.q	r15, SFO(SF_FLAGS,sz), r63				      ;\
	st.q	r15, IFO(IF_R3,sz), r3	/* Save r3 - r13 */		      ;\
	st.q	r15, IFO(IF_R4,sz), r4					      ;\
	st.q	r15, IFO(IF_R5,sz), r5					      ;\
	st.q	r15, IFO(IF_R6,sz), r6					      ;\
	st.q	r15, IFO(IF_R7,sz), r7					      ;\
	st.q	r15, IFO(IF_R8,sz), r8					      ;\
	st.q	r15, IFO(IF_R9,sz), r9					      ;\
	st.q	r15, IFO(IF_R10,sz), r10				      ;\
	st.q	r15, IFO(IF_R11,sz), r11				      ;\
	st.q	r15, IFO(IF_R12,sz), r12				      ;\
	st.q	r15, IFO(IF_R13,sz), r13				      ;\
	st.q	r15, IFO(IF_R14,sz), r14/* Save r14 (frame pointer) */	      ;\
	st.q	r15, IFO(IF_R16,sz), r16/* Save r16 - r23 */		      ;\
	st.q	r15, IFO(IF_R17,sz), r17				      ;\
	st.q	r15, IFO(IF_R18,sz), r18				      ;\
	st.q	r15, IFO(IF_R19,sz), r19				      ;\
	st.q	r15, IFO(IF_R20,sz), r20				      ;\
	st.q	r15, IFO(IF_R21,sz), r21				      ;\
	st.q	r15, IFO(IF_R22,sz), r22				      ;\
	st.q	r15, IFO(IF_R23,sz), r23				      ;\
	st.q	r15, IFO(IF_R25,sz), r25/* Save r25 - r27 */		      ;\
	st.q	r15, IFO(IF_R26,sz), r26				      ;\
	st.q	r15, IFO(IF_R27,sz), r27				      ;\
	st.q	r15, IFO(IF_R36,sz), r36/* Save r36 - r43 */		      ;\
	st.q	r15, IFO(IF_R37,sz), r37				      ;\
	st.q	r15, IFO(IF_R38,sz), r38				      ;\
	st.q	r15, IFO(IF_R39,sz), r39				      ;\
	st.q	r15, IFO(IF_R40,sz), r40				      ;\
	st.q	r15, IFO(IF_R41,sz), r41				      ;\
	st.q	r15, IFO(IF_R42,sz), r42				      ;\
	st.q	r15, IFO(IF_R43,sz), r43				      ;\
	st.q	r15, IFO(IF_R60,sz), r60/* Save r60 - r62 */		      ;\
	st.q	r15, IFO(IF_R61,sz), r61				      ;\
	st.q	r15, IFO(IF_R62,sz), r62				      ;\
	gettr	tr1, r0			/* Save tr1 - tr4 (tr0 already svd) */;\
	st.q	r15, IFO(IF_TR1,sz), r0					      ;\
	gettr	tr2, r0							      ;\
	st.q	r15, IFO(IF_TR2,sz), r0					      ;\
	gettr	tr3, r0							      ;\
	st.q	r15, IFO(IF_TR3,sz), r0					      ;\
	gettr	tr4, r0							      ;\
	st.q	r15, IFO(IF_TR4,sz), r0					      ;\
	or	r15, r63, r14		/* Fix up the frame pointer */

/*
 * Restore caller-saved registers from an interrupt frame
 *
 * The `sz' parameter specifies the size of the current stack frame.
 * This will be either SZ_INTRFRAME or SZ_TRAPFRAME.
 *
 * Note: Assumes r0, r1, r2 and tr0 will be restored by _EXCEPTION_EXIT macro.
 */
#define	_INTR_FRAME_RESTORE(sz)						      \
	ld.q	r15, IFO(IF_R3,sz), r3	/* Restore r3 - r13 */		      ;\
	ld.q	r15, IFO(IF_R4,sz), r4					      ;\
	ld.q	r15, IFO(IF_R5,sz), r5					      ;\
	ld.q	r15, IFO(IF_R6,sz), r6					      ;\
	ld.q	r15, IFO(IF_R7,sz), r7					      ;\
	ld.q	r15, IFO(IF_R8,sz), r8					      ;\
	ld.q	r15, IFO(IF_R9,sz), r9					      ;\
	ld.q	r15, IFO(IF_R10,sz), r10				      ;\
	ld.q	r15, IFO(IF_R11,sz), r11				      ;\
	ld.q	r15, IFO(IF_R12,sz), r12				      ;\
	ld.q	r15, IFO(IF_R13,sz), r13				      ;\
	ld.q	r15, IFO(IF_R14,sz), r14/* Restore r14 */		      ;\
	ld.q	r15, IFO(IF_R16,sz), r16/* Restore r16 - r23 */		      ;\
	ld.q	r15, IFO(IF_R17,sz), r17				      ;\
	ld.q	r15, IFO(IF_R18,sz), r18				      ;\
	ld.q	r15, IFO(IF_R19,sz), r19				      ;\
	ld.q	r15, IFO(IF_R20,sz), r20				      ;\
	ld.q	r15, IFO(IF_R21,sz), r21				      ;\
	ld.q	r15, IFO(IF_R22,sz), r22				      ;\
	ld.q	r15, IFO(IF_R23,sz), r23				      ;\
	ld.q	r15, IFO(IF_R25,sz), r25/* Restore r25 - r27 */		      ;\
	ld.q	r15, IFO(IF_R26,sz), r26				      ;\
	ld.q	r15, IFO(IF_R27,sz), r27				      ;\
	ld.q	r15, IFO(IF_R36,sz), r36/* Restore r36 - r43 */		      ;\
	ld.q	r15, IFO(IF_R37,sz), r37				      ;\
	ld.q	r15, IFO(IF_R38,sz), r38				      ;\
	ld.q	r15, IFO(IF_R39,sz), r39				      ;\
	ld.q	r15, IFO(IF_R40,sz), r40				      ;\
	ld.q	r15, IFO(IF_R41,sz), r41				      ;\
	ld.q	r15, IFO(IF_R42,sz), r42				      ;\
	ld.q	r15, IFO(IF_R43,sz), r43				      ;\
	ld.q	r15, IFO(IF_R60,sz), r60/* Restore r60 - r62 */		      ;\
	ld.q	r15, IFO(IF_R61,sz), r61				      ;\
	ld.q	r15, IFO(IF_R62,sz), r62				      ;\
	ld.q	r15, IFO(IF_TR0,sz), r0	/* Restore tr0 - tr4 */		      ;\
	ptabs/u	r0, tr0							      ;\
	ld.q	r15, IFO(IF_TR1,sz), r0					      ;\
	ptabs/u	r0, tr1							      ;\
	ld.q	r15, IFO(IF_TR2,sz), r0					      ;\
	ptabs/u	r0, tr2							      ;\
	ld.q	r15, IFO(IF_TR3,sz), r0					      ;\
	ptabs/u	r0, tr3							      ;\
	ld.q	r15, IFO(IF_TR4,sz), r0					      ;\
	ptabs/u	r0, tr4


/*
 * Save a trap frame on the stack (basically, callee-saved registers).
 *
 * The `sz' parameter specifies the size of the current stack frame.
 * This should always be SZ_TRAPFRAME.
 */
#define	_TRAP_FRAME_SAVE(sz)						      \
	movi	SF_FLAGS_CALLEE_SAVED, r0				      ;\
	st.q	r15, SFO(SF_FLAGS,sz), r0				      ;\
	st.q	r15, TFO(TF_R28,sz), r28 /* Save r28 - r35 */		      ;\
	st.q	r15, TFO(TF_R29,sz), r29				      ;\
	st.q	r15, TFO(TF_R30,sz), r30				      ;\
	st.q	r15, TFO(TF_R31,sz), r31				      ;\
	st.q	r15, TFO(TF_R32,sz), r32				      ;\
	st.q	r15, TFO(TF_R33,sz), r33				      ;\
	st.q	r15, TFO(TF_R34,sz), r34				      ;\
	st.q	r15, TFO(TF_R35,sz), r35				      ;\
	st.q	r15, TFO(TF_R44,sz), r44 /* Save r44 - r59 */		      ;\
	st.q	r15, TFO(TF_R45,sz), r45				      ;\
	st.q	r15, TFO(TF_R46,sz), r46				      ;\
	st.q	r15, TFO(TF_R47,sz), r47				      ;\
	st.q	r15, TFO(TF_R48,sz), r48				      ;\
	st.q	r15, TFO(TF_R49,sz), r49				      ;\
	st.q	r15, TFO(TF_R50,sz), r50				      ;\
	st.q	r15, TFO(TF_R51,sz), r51				      ;\
	st.q	r15, TFO(TF_R52,sz), r52				      ;\
	st.q	r15, TFO(TF_R53,sz), r53				      ;\
	st.q	r15, TFO(TF_R54,sz), r54				      ;\
	st.q	r15, TFO(TF_R55,sz), r55				      ;\
	st.q	r15, TFO(TF_R56,sz), r56				      ;\
	st.q	r15, TFO(TF_R57,sz), r57				      ;\
	st.q	r15, TFO(TF_R58,sz), r58				      ;\
	st.q	r15, TFO(TF_R59,sz), r59				      ;\
	gettr	tr5, r0							      ;\
	st.q	r15, TFO(TF_TR5,sz), r0					      ;\
	gettr	tr6, r0							      ;\
	st.q	r15, TFO(TF_TR6,sz), r0					      ;\
	gettr	tr7, r0							      ;\
	st.q	r15, TFO(TF_TR7,sz), r0

/*
 * Restore callee-saved registers from a trap frame
 *
 * The `sz' parameter specifies the size of the current stack frame.
 * This should always be SZ_TRAPFRAME.
 */
#define	_TRAP_FRAME_RESTORE(sz)						      \
	pta/u	99f, tr0						      ;\
	ld.q	r15, SFO(SF_FLAGS,sz), r0				      ;\
	beq/u	r0, r63, tr0						      ;\
	ld.q	r15, TFO(TF_R28,sz), r28 /* Restore r28 - r35 */	      ;\
	ld.q	r15, TFO(TF_R29,sz), r29				      ;\
	ld.q	r15, TFO(TF_R30,sz), r30				      ;\
	ld.q	r15, TFO(TF_R31,sz), r31				      ;\
	ld.q	r15, TFO(TF_R32,sz), r32				      ;\
	ld.q	r15, TFO(TF_R33,sz), r33				      ;\
	ld.q	r15, TFO(TF_R34,sz), r34				      ;\
	ld.q	r15, TFO(TF_R35,sz), r35				      ;\
	ld.q	r15, TFO(TF_R44,sz), r44 /* Restore r44 - r59 */	      ;\
	ld.q	r15, TFO(TF_R45,sz), r45				      ;\
	ld.q	r15, TFO(TF_R46,sz), r46				      ;\
	ld.q	r15, TFO(TF_R47,sz), r47				      ;\
	ld.q	r15, TFO(TF_R48,sz), r48				      ;\
	ld.q	r15, TFO(TF_R49,sz), r49				      ;\
	ld.q	r15, TFO(TF_R50,sz), r50				      ;\
	ld.q	r15, TFO(TF_R51,sz), r51				      ;\
	ld.q	r15, TFO(TF_R52,sz), r52				      ;\
	ld.q	r15, TFO(TF_R53,sz), r53				      ;\
	ld.q	r15, TFO(TF_R54,sz), r54				      ;\
	ld.q	r15, TFO(TF_R55,sz), r55				      ;\
	ld.q	r15, TFO(TF_R56,sz), r56				      ;\
	ld.q	r15, TFO(TF_R57,sz), r57				      ;\
	ld.q	r15, TFO(TF_R58,sz), r58				      ;\
	ld.q	r15, TFO(TF_R59,sz), r59				      ;\
	ld.q	r15, TFO(TF_TR5,sz), r0					      ;\
	ptabs/u	r0, tr5							      ;\
	ld.q	r15, TFO(TF_TR6,sz), r0					      ;\
	ptabs/u	r0, tr6							      ;\
	ld.q	r15, TFO(TF_TR7,sz), r0					      ;\
	ptabs/u	r0, tr7							      ;\
99:




/*========================= Start of Vector Table ============================*/

	_TEXT_SECTION		/* XXX: from <sh5/asm.h> */
	.balign 0x100

GLOBAL(sh5_vector_table)
	.space	(0x100 - (. - _C_LABEL(sh5_vector_table)))


/******************************************************************************
 * Non-TLB Miss Synchronous Exception Handler
 *
 * VBR Offset: 0x100
 *     Length: 0x100
 *
 * XXX: This handler very nearly fills the 0x100 byte exception slot :XXX
 */
Lsh5_vector_general:
	_EXCEPTION_ENTRY(SZ_TRAPFRAME, CRIT_SYNC_EXCEPTION)
	pta/l	Lsh5_event_sync, tr0
	blink	tr0, r63
	nop
	nop
	nop
	nop
	nop

	.space	(0x300 - (. - Lsh5_vector_general))


/******************************************************************************
 * TLB Miss Exception Handler
 *
 * VBR Offset: 0x400
 *     Length: 0x200
 *
 * TLB miss is special.
 *
 * We can't use the normal _EXCEPTION_ENTRY sequence as we can be called
 * during some other exception's critical section.
 *
 * Basically, save the current SP in KCR1, switch to the TLB stack in
 * KSEG0, free up some registers and deal with the miss.
 *
 * It is *not* safe to unblock exceptions in this code.
 *
 * Note: Byte counts assume LP64. ILP32 will be slightly smaller.
 *       Addendum: bytes counts somewhat out of date ...
 */
#define	TS(o)		((o)+CI_TSCRATCH)

Lsh5_vector_tlbmiss:
	getcon	usr, r24		/* Save USR in r24 for the duration */
	putcon	r15, kcr1		/* Save current SP */
	getcon	kcr0, r15
	st.q	r15, TS(TS_R0), r0	/* Free up some registers */
	st.q	r15, TS(TS_R1), r1
	st.q	r15, TS(TS_R2), r2
	st.q	r15, TS(TS_R3), r3
	st.q	r15, TS(TS_R4), r4
	st.q	r15, TS(TS_R5), r5
	st.q	r15, TS(TS_R6), r6
	gettr	tr0, r0
	st.q	r15, TS(TS_TR0), r0
	gettr	tr1, r0
	st.q	r15, TS(TS_TR1), r0
	movi	TS(TS_STACK), r0
	add	r15, r0, r15

	getcon	tea, r0			/* VA which missed -> r0 */
	shari	r0, PGSHIFT, r0		/* Get missing VPN */

	/* Determine whether the access was kernel or user-mode */

	getcon	ssr, r1			/* r1 = SR at time of miss */
	shlri	r1, SH5_CONREG_SR_MD_SHIFT, r1
	andi	r1, 1, r1		/* r1 = 0 if came from user mode */
	pta/u	Ltlbmiss_user_ipt, tr0
	beq/u	r1, r63, tr0		/* Jump if user-mode TLB miss */

	/* The miss happened in kernel-mode */

#ifndef _LP64
	LDSC32(SH5_KSEG1_BASE, r1)
#else
	LDSC64(SH5_KSEG1_BASE, r1)
#endif
	shari	r1, PGSHIFT, r1
	bgtu/u	r1, r0, tr0		/* Jump if not in KSEG1 */

	/* Ok, we're dealing with a kernel-mode KSEG1 access */

	sub	r0, r1, r0		/* Convert to IPT index */
	pta/u	Ltlbmiss_dotrap, tr0
	LDC32(KERNEL_IPT_SIZE-1, r1)
	bgtu/u	r0, r1, tr0		/* Jump if KVA is invalid */

	/* The access was somewhere inside managed KVA space */

	LEA(_C_LABEL(pmap_kernel_ipt), r4) /* Base of KVA IPT */

#if SH5_NEFF_BITS > 32
	shlli	r0, 3, r1		/* VPN * sizeof(*pmap_kernel_ipt) */
	shlli	r0, 2, r0
	add	r0, r1, r0
	add	r4, r0, r4
	ldlo.q	r4, SH5_PTE_PTEL, r1	/* Get the PTEL for this page */
	ldhi.q	r4, SH5_PTE_PTEL + 7, r0
	or	r0, r1, r1
#else
	shlli	r0, 2, r1		/* VPN * sizeof(*pmap_kernel_ipt) */
	shlli	r0, 1, r0
	add	r0, r1, r0
	add	r4, r0, r4
	ldlo.l	r4, SH5_PTE_PTEL, r1	/* Get the PTEL for this page */
	ldhi.l	r4, SH5_PTE_PTEL + 3, r0
	or	r0, r1, r1
#endif

	movi	SH5_PTEL_RM_MASK, r0
	andc	r1, r0, r1
	beq/u	r1, r63, tr0		/* Jump if KVA not mapped */

	/*
	 * We have a valid PTEL entry for the page.
	 * Now synthesise a PTEH entry.
	 */

	getcon	tea, r0			/* Re-fetch missed address */
	movi	NBPG-1, r2
	andc	r0, r2, r0		/* Keep only VPN part (ASID=0) */
	ori	r0, (SH5_PTEH_SH | SH5_PTEH_V), r0 /* Set SH and V bits */

	/*
	 * Invoke the cpu-specific TLB fill subroutine.
	 *
	 * It expects the following registers to be live:
	 *
	 *  r0 - PTEH value for the missing mapping
	 *  r1 - PTEL value for the missing mapping
	 *  r2 - Return Address
	 *
	 * It requires the following registers to be available:
	 *
	 *  r3, tr0, and tr1.
	 *
	 * If the PTE's protection bits are incompatible with the type of
	 * TLB miss (e.g. ITLB miss, but non-executable mapping) the
	 * subroutine will vector off to Ltlbmiss_dotrap instead
	 * of returning.
	 */
Ltlbmiss_updatetlb:
	LEA(_C_LABEL(__cpu_tlbload), r2)
	LDPTR	r2, 0, r2
	ptabs/l	r2, tr0
	blink	tr0, r2

	/*
	 * The TLB has been updated.
	 * We know for certain that the access will succeed when we
	 * return to the exception site, so we can safely update the
	 * pmap's referenced/modified attributes for the page.
	 */
	getcon	expevt, r0		/* Test for Write TLB Miss */
	movi	SH5_PTEL_M, r1		/* Presume "modified" */
	addi	r0, -T_WTLBMISS, r0	/* r0 == 0 for Writes */
	cmvne	r0, r63, r1		/* If r0 != 0, Clear "modified" */
	movi	SH5_PTEL_R, r0		/* Referenced bit is always set */
	or	r0, r1, r1		/* r1 = Ref/Mod bits */

#if SH5_NEFF_BITS > 32
	ldlo.q	r4, SH5_PTE_PTEL, r0	/* Get the PTEL for this page */
	ldhi.q	r4, SH5_PTE_PTEL + 7, r2
	or	r0, r2, r0
#else
	ldlo.l	r4, SH5_PTE_PTEL, r0	/* Get the PTEL for this page */
	ldhi.l	r4, SH5_PTE_PTEL + 3, r2
	or	r0, r2, r0
#endif
	or	r0, r1, r1		/* OR in the ref/mod bits */

#if SH5_NEFF_BITS > 32
	stlo.q	r4, SH5_PTE_PTEL, r1	/* Update the PTE */
	sthi.q	r4, SH5_PTE_PTEL + 7, r1
#else
	stlo.l	r4, SH5_PTE_PTEL, r1	/* Update the PTE */
	sthi.l	r4, SH5_PTE_PTEL + 3, r1
#endif

	getcon	kcr0, r15
	ld.q	r15, TS(TS_TR0), r0	/* Restore scratch registers */
	ptabs/u	r0, tr0
	ld.q	r15, TS(TS_TR1), r0
	ptabs/u	r0, tr1
	ld.q	r15, TS(TS_R0), r0
	ld.q	r15, TS(TS_R1), r1
	ld.q	r15, TS(TS_R2), r2
	ld.q	r15, TS(TS_R3), r3
	ld.q	r15, TS(TS_R4), r4
	ld.q	r15, TS(TS_R5), r5
	ld.q	r15, TS(TS_R6), r6
	getcon	kcr1, r15		/* Restore SP */
	putcon	r24, usr
	synco
	rte				/* Return to previous context */
	nop
	nop
	nop
	nop
	nop

	.space	(0x200 - (. - Lsh5_vector_tlbmiss))


/******************************************************************************
 * Non-Debug Asynchronous Hardware Interrupt Exception
 *
 * VBR Offset: 0x600
 *     Length: 0x100  (???)
 *
 * Asynchronous Exception Handler (Hardware interrupts to you and me)
 */
Lsh5_vector_interrupt:
	_EXCEPTION_ENTRY(SZ_INTRFRAME, CRIT_ASYNC_EXCEPTION)
	ld.q	r2, ES(ES_INTEVT), r0	/* Fetch interrupt vector */
	st.q	r15, SFO(SF_INTEVT, SZ_INTRFRAME), r0 /* Save in stateframe */
	st.q	r2, ES(ES_CRITICAL), r63 /* Left the critical section */
	ld.l	r2, CI_INTR_DEPTH, r0	/* Update interrupt nesting level */
	addi	r0, 1, r0
	st.l	r2, CI_INTR_DEPTH, r0
	putcon	r1, sr			/* Can safely take interrupts now */
	pta/l	Lsh5_event_interrupt, tr0
	blink	tr0, r63

	.balign 0x10

/*======================== End of VBR Vector Table ===========================*/


/******************************************************************************
 * Continuation of Synchronous Events.
 *
 * A state-frame has been saved, and we're out of the critical section.
 *
 * This code is used for trapa as well as regular synchronous
 * exceptions. This isn't really ideal as we end up saving more state
 * on the stack than is necessary.
 *
 * So, there are a couple of optimisations in the pipeline for trapa:
 *
 *   - Save only a very small subset of registers initially.
 *     (Over and above those saved by _INTR_FRAME_SAVE()).
 *     First thoughts on this would be r1, r2-r14, r15, r18, r26, r27
 *
 *   - Invoke trapa().
 *
 *   - If trapa() returns zero, restore the minimal context and check
 *     for softints/asts in the normal way.
 *
 *   - If trapa() returns non-zero, that's our signal to save a *full*
 *     context in the trapframe and re-invoke trapa(). This would happen
 *     if a system call requires a complete trapframe because it is
 *     one of fork(2), vfork(2), or any other syscall which short-circuits
 *     the normal call stack and returns via proc_trampoline.
 *
 *   - The above will require an additional flag for struct sysent's
 *     sy_flags, SYCALL_FULL_CTX (for example), so that trapa() can
 *     detect when the full context is required.
 */
Lsh5_event_sync:
	ld.q	r2, ES(ES_EXPEVT), r0	/* Fetch contingent data */
	st.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0 /* Save in stateframe */
	ld.q	r2, ES(ES_TEA), r0
	st.q	r15, SFO(SF_TEA, SZ_TRAPFRAME), r0
	ld.q	r2, ES(ES_TRA), r0
	st.q	r15, SFO(SF_TRA, SZ_TRAPFRAME), r0
	st.q	r2, ES(ES_CRITICAL), r63 /* Left the critical section */
	putcon	r1, sr			/* Can safely take interrupts now */
	_INTR_FRAME_SAVE(SZ_TRAPFRAME)
	_TRAP_FRAME_SAVE(SZ_TRAPFRAME)

	ld.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0 /* Fetch exception type */
	pta/l	Ltrapagain, tr0
	addi	r0, -T_TRAP, r0		/* See if exception caused by "trapa" */
	bne/l	r0, r63, tr0		/* Jump if not */
	LEAF(_C_LABEL(trapa), r0)
	LDPTR	r2, CI_CURLWP, r2	/* Get lwp pointer */
	ptabs/l	r0, tr0
	or	r15, r63, r3
	blink	tr0, r18		/* trapa(curlwp, trapframe); */
	pta/l	Ltrapepilogue, tr0
	blink	tr0, r63

Ltrapagain:
	getcon	kcr0, r2		/* Get cpu_info for this cpu */
	LEAF(_C_LABEL(trap), r0)
	LDPTR	r2, CI_CURLWP, r2	/* Get lwp pointer */
	ptabs/l	r0, tr0
	or	r15, r63, r3
	blink	tr0, r18		/* trap(curlwp, trapframe); */

Ltrapepilogue:
	/* Check for ASTs */
	pta/l	Ltrapexit, tr0		/* Preload the No-AST path */
	ld.l	r15, TF_SSR, r2		/* Fetch saved Status Register */
	shlri	r2, SH5_CONREG_SR_MD_SHIFT, r0
	andi	r0, 1, r0		/* Going back to user-mode? */
	bne/u	r0, r63, tr0		/* Nope. No need to check for ASTs */
	getcon	kcr0, r1		/* Get cpu_info for this cpu */
	LDPTR	r1, CI_CURLWP, r1	/* Get curlwp */
	beq/u	r1, r63, tr0		/* Exit if NULL */
	LDPTR	r1, L_PROC, r1		/* r1 = l->l_proc */
#ifndef _LP64
	addi	r1, P_MD_ASTPENDING, r1	/* Point to md_astpending */
#else
	movi	P_MD_ASTPENDING, r2	/* Exceeds 10-bit field in _LP64 mode */
	add	r1, r2, r1		/* Point to md_astpending */
#endif
	swap.q	r1, r63, r0		/* AST pending for this process? */
	beq/l	r0, r63, tr0		/* Nope. Moving right along ... */
	pta/l	Ltrapagain, tr0
	movi	T_AST, r0		/* Fabricate an "AST" event */
	st.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0
	blink	tr0, r63		/* Go back around to deal with it */

Ltrapexit:
#if defined(PORTMASTER)
	movi	SH5_CONREG_SR_IMASK_ALL, r0
	putcon	r0, sr
	LEAF(_C_LABEL(validate_trapframe), r0)
	ptabs/l	r0, tr0
	or	r15, r63, r2
	blink	tr0, r18
#endif
	_TRAP_FRAME_RESTORE(SZ_TRAPFRAME)
	_INTR_FRAME_RESTORE(SZ_TRAPFRAME)
	_EXCEPTION_EXIT(SZ_TRAPFRAME, CRIT_SYNC_EXCEPTION)
	/* NOTREACHED */

/******************************************************************************
 *
 * Hardware Interrupt Handler
 *
 * A state-frame has been saved, and we're out of the critical section.
 * Space has been allocated on the stack for an interrupt frame, so save
 * caller-saved registers into it before invoking the C dispatcher.
 */
Lsh5_event_interrupt:
	_INTR_FRAME_SAVE(SZ_INTRFRAME)

#ifdef DDB
	/*
	 * Catch NMIs early so that we can convert the interrupt frame
	 * to a regular trapframe and drop into DDB if the NMI button
	 * is pressed.
	 *
	 * This provides a handy way to regain control if the CPU should
	 * decide to spin at a high ipl.
	 */
	pta/u	Lsh5_event_nmi, tr0
	ld.q	r15, SFO(SF_INTEVT, SZ_INTRFRAME), r0 /* Fetch INTEVT */
	movi	0x1c0, r1		/* The NMI event code */
	beq/u	r0, r1, tr0		/* Jump if NMI */
#endif

	/* sh5_intr_dispatch(struct intrframe *f); */
	LEAF(_C_LABEL(sh5_intr_dispatch), r0)
	ptabs/l	r0, tr0
	or	r15, r63, r2
	blink	tr0, r18

	/* Check for Software Interrupts */
	LEAF(Lcheck_softint, r0)
	ptabs/l	r0, tr0
	ld.q	r15, SFO(SF_SSR, SZ_INTRFRAME), r2
	blink	tr0, r18

	/* Update interrupt nesting level */
	getcon	sr, r1			/* Need to disable interrupts */
	ori	r1, SH5_CONREG_SR_IMASK_ALL, r0
	putcon	r0, sr
	getcon	kcr0, r2		/* Get cpu_info for this cpu */
	ld.l	r2, CI_INTR_DEPTH, r0	/* Fetch interrupt nesting level */
	addi	r0, -1, r0		/* One less level */
	st.l	r2, CI_INTR_DEPTH, r0	/* Write it back */
	putcon	r1, sr			/* Restore interrupts */

	/* Check for ASTs */
	pta/l	Lintrexit, tr0		/* Preload the No-AST path */
	ld.l	r15, IF_SSR, r2		/* Fetch saved Status Register */
	shlri	r2, SH5_CONREG_SR_MD_SHIFT, r0
	andi	r0, 1, r0		/* Going back to user-mode? */
	bne/u	r0, r63, tr0		/* Nope. No need to check for ASTs */
	getcon	kcr0, r1		/* Get cpu_info for this cpu */
	LDPTR	r1, CI_CURLWP, r1	/* Get curlwp */
	beq/u	r1, r63, tr0		/* Exit if NULL */
	LDPTR	r1, L_PROC, r1		/* r1 = l->l_proc */
#ifndef _LP64
	addi	r1, P_MD_ASTPENDING, r1	/* Point to md_astpending */
#else
	movi	P_MD_ASTPENDING, r2	/* Exceeds 10-bit field in _LP64 mode */
	add	r1, r2, r1		/* Point to md_astpending */
#endif
	swap.q	r1, r63, r0		/* ASTs pending for this process? */
	beq/l	r0, r63, tr0		/* Nope. Moving right along ... */

	/*
	 * We now need to re-vector through the normal synchronous
	 * exception handler to deal with the AST trap. However, we're
	 * currently running on an interrupt stack frame, so we need
	 * to convert this to an exception stack frame.
	 *
	 * Fortunately, the frame layout has been designed such that all
	 * we need to do is add (SZ_TRAPFRAME - SZ_INTRFRAME) to the
	 * current stack pointer, et voila! We have a trapframe!
	 *
	 * The caller-saved registers and machine state map exactly from
	 * the interrupt frame to the trap frame, so there's no need
	 * to re-save them.
	 */
	addi	r15, -(SZ_TRAPFRAME - SZ_INTRFRAME), r15
	_TRAP_FRAME_SAVE(SZ_TRAPFRAME)
	add	r15, r63, r14		/* Adjust the frame-pointer too */
	getcon	sr, r0
	movi	SH5_CONREG_SR_IMASK_ALL, r1
	andc	r0, r1, r0
	putcon	r0, sr			/* Drop to ipl 0 */
	pta/l	Ltrapagain, tr0
	movi	T_AST, r0		/* Fabricate an "AST" event */
	st.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0
	blink	tr0, r63		/* Go back around to deal with it */

Lintrexit:
	_INTR_FRAME_RESTORE(SZ_INTRFRAME)
	_EXCEPTION_EXIT(SZ_INTRFRAME, CRIT_ASYNC_EXCEPTION)
	/* NOTREACHED */


#ifdef DDB
Lsh5_event_nmi:
	/*
	 * Convert the interrupt frame to a trapframe
	 */
	addi	r15, -(SZ_TRAPFRAME - SZ_INTRFRAME), r15
	_TRAP_FRAME_SAVE(SZ_TRAPFRAME)
	add	r15, r63, r14		/* Adjust the frame-pointer too */
	pta/l	Ltrapagain, tr0
	movi	T_NMI, r0		/* Fabricate an "NMI" event */
	st.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0
	blink	tr0, r63		/* Go back around to deal with it */
#endif /* DDB */


/******************************************************************************
 *
 * Non-KSEG1 TLB Miss Handlers
 *
 *****************************************************************************/

/******************************************************************************
 * Convenience macro for generating a pmap_pteg_table hash given an VSID and
 * Virtual Page Number.
 *
 * `vsid' is not preserved. The hash is returned in this register.
 * `vpn' is not preserved.
 * `bits' should be set to pmap_pteg_bits. On exit, it
 * will be replaced with pmap_pteg_mask.
 */
#define	MAKE_PTEG_HASH(vsid, vpn, bits)				       \
	pta/u	1f, tr0							      ;\
1:	xor	vpn, vsid, vsid		/* XOR `pmap_pteg_bits' of the VPN */ ;\
	shlrd	vpn, bits, vpn		/* Get next chunk of VPN bits */      ;\
	bne/l	vpn, r63, tr0		/* Back until VPN goes to zero. */    ;\
	movi	1, vpn			/* Generate pmap_pteg_mask */	      ;\
	shlld	vpn, bits, vpn						      ;\
	addi	vpn, -1, bits						      ;\
	and	vsid, bits, vsid


/******************************************************************************
 *
 * u_int pmap_ipt_hash(vsid_t vsid, vaddr_t va)
 *
 * In an effort to keep the hash algorithm in one place...
 *
 * r2 == vsid,
 * r3 == va
 */
ENTRY_NOPROFILE(pmap_ipt_hash)
#ifndef _LP64
	add.l	r3, r63, r3		/* Sign extend */
#endif
	shari	r3, PGSHIFT, r3		/* Convert va to EPN */
	LEA(_C_LABEL(pmap_pteg_bits), r0)
	ld.l	r0, 0, r0		/* # of bits set in pmap_pteg_mask */
	MAKE_PTEG_HASH(r2, r3, r0)	/* Generate the hash */
	ptabs/l	r18, tr0
	blink	tr0, r63		/* All done */


/******************************************************************************
 *
 * TLB miss handler for pages which should be looked up in pmap_pteg_table.
 *
 * Kernel mode KSEG1 TLB misses have already been dealt with. We now have
 * to deal with misses which require looking up in the "pmap_pteg_table".
 * This is somewhat harder as it requires we hash the VSID/VPN and deal with
 * any hash colisions which occur.
 *
 * At this point:
 *
 *	r0 == The VPN which caused the TLB miss
 *	r1, r2, r3, r4, and tr0: Available
 */
Ltlbmiss_user_ipt:
	pta/u	Ltlbmiss_dotrap, tr0	/* Short-circuit NULL dereferences */
	beq/u	r0, r63, tr0

	/* Get the VSID for the current pmap */

	getcon	kcr0, r2		/* Get cpu_info */
	ld.uw	r2, CI_CURVSID, r2	/* Fetch current vsid */
	or	r2, r63, r5		/* Save vsid in r5 */
	LEA(_C_LABEL(pmap_pteg_bits), r3)
	ld.l	r3, 0, r3
	MAKE_PTEG_HASH(r2, r0, r3)
	or	r2, r63, r3		/* Save hash in r3 (in case of spill) */
	HASH_TO_PTEG_IDX(r2, r1)	/* Generate pteg table offset */
	LEA(_C_LABEL(pmap_pteg_table), r0)
	LDPTR	r0, 0, r0
	add	r2, r0, r2		/* r2 points to required PTE group */
	getcon	tea, r4			/* Get missed virtual address */

	/*
	 * r0/r1 available
	 * r2    PTE group to search
	 * r3    Hash
	 * r4    The EPN we're looking for.
	 * r5    The VSID we're looking for.
	 *
	 * XXX: Register allocation could be optimised a wee bit here...
	 */
	pta/l	1f, tr0
	pta/l	2f, tr1

	movi	SH5_PTE_PN_MASK_MOVI, r1/* Mask for EPN */
	movi	SH5_PTEG_SIZE, r6	/* How many PTEs per group */
	and	r4, r1, r4
1:	ld.uw	r2, SH5_PTE_VSID, r0	/* Fetch VSID */
	bne/u	r5, r0, tr1		/* Jump if PTE's VSID doesn't match */
	LDPTE	r2, SH5_PTE_PTEH, r0	/* Fetch PTEH */
	and	r0, r1, r0		/* Keep only EPN bits */
	bne/u	r4, r0, tr1		/* Jump if PTE's EPN doesn't match */
	pta/l	Ltlbmiss_gotpte, tr0	/* Got it. */
	blink	tr0, r63
2:	addi	r2, SZ_SH5_PTE_T, r2	/* Next PTE in the group */
	addi	r6, -1, r6
	bne/l	r6, r63, tr0		/* Back for all 8 */

	pta/l	Ltlbmiss_nomatch, tr0
	blink	tr0, r63		/* No match found */

	/*
	 * Found the required mapping in the pteg table.
	 *
	 * r2 = &pmap_pteg_table[hash].pte[required_pte]
	 * r0 = PTEH from the above pte
	 */
Ltlbmiss_gotpte:
	LDPTE	r2, SH5_PTE_PTEL, r1	/* Fetch PTEL */
	movi	SH5_PTEL_RM_MASK, r3
	andc	r1, r3, r1

	/* First of all, catch user-mode accesses to privileged addresses */

	getcon	ssr, r3			/* r3 = SR at time of miss */
	shlri	r3, SH5_CONREG_SR_MD_SHIFT, r3	/* Get SSR.MD to bit#0 */
	shlri	r1, SH5_PTEL_PR_U_SHIFT, r4	/* Get page-prot bit */
	or	r3, r4, r3		/* Combine privilege bits */
	andi	r3, 1, r3
	pta/u	Ltlbmiss_dotrap, tr0
	beq/u	r3, r63, tr0		/* Jump if access is disallowed */

	/* Update PTEH with the current ASID */

	getcon	ssr, r3			/* Get ASID in effect at TLB miss time*/
	shlri	r3, SH5_CONREG_SR_ASID_SHIFT, r3
	andi	r3, SH5_CONREG_SR_ASID_MASK, r3
	shlli	r3, SH5_PTEH_ASID_SHIFT, r3
	movi	SH5_PTE_PN_MASK_MOVI, r4
	pta/l	Ltlbmiss_updatetlb, tr0
	and	r0, r4, r0
	or	r3, r0, r0		/* Now have correct PTEH value */
	or	r2, r63, r4		/* Preserve PTE pointer in r4 */
	blink	tr0, r63

	/*
	 * No match for the mapping in pmap_pteg_table. Try to spill a
	 * pte from the overflow table.
	 *
	 * r3 = Hash (PTEG index)
	 * r4 = EPN (Shift left by PGSHIFT, so effectively a Virtual Address)
	 * r5 = VSID
	 *
	 * pte = pmap_pte_spill(u_int ptegidx, u_int vsid, vaddr_t va);
	 */
Ltlbmiss_nomatch:
	addi	r15, -SZ_INTRFRAME, r15
	_INTR_FRAME_SAVE(SZ_INTRFRAME)	/* Use this to do Caller-Save */
	LEAF(_C_LABEL(pmap_pte_spill), r1)
	ptabs/l	r1, tr0
	/* Get parameters to correct regsisters */
	or	r3, r63, r2		/* r2 = ptegidx */
	or	r5, r63, r3		/* r3 = vsid */
					/* r4 = va (already) */
	blink	tr0, r18		/* Make the call */
	or	r2, r63, r1		/* Get return value to r1 */
	_INTR_FRAME_RESTORE(SZ_INTRFRAME) /* Restore Caller-Save registers */
	addi	r15, SZ_INTRFRAME, r15	/* Restore stack */
	or	r1, r63, r2
	pta/u	Ltlbmiss_dotrap, tr0
	beq/u	r1, r63, tr0
	pta/l	Ltlbmiss_gotpte, tr0
	LDPTE	r2, SH5_PTE_PTEH, r0
	blink	tr0, r63

	/*
	 * We come here for one of three reasons:
	 *
	 *  1. No mapping was found for the required virtual address,
	 *     either in the pmap_pteg_table or the pmap_kva_ipt.
	 *
	 *  2. A mapping *was* found, but the fault was one of:
	 *
	 *     . a Data TLB miss, but the page is neither readable nor writable
	 *       (This is only checked if DEBUG is defined)
	 *     . an Instruction TLB miss, but the page isn't marked executable
	 *     . a user-mode access to a privileged page.
	 *
	 *  3. Any access to page 0, which would indicate a NULL pointer
	 *     de-reference.
	 *
	 * For all cases, we have to defer the fault to the regular exception
	 * handler.
	 */
Ltlbmiss_dotrap:
	/*
	 * Set things up to take a normal synchronous exception, which means
	 * we have to restore all registers to their pre-TLB miss state.
	 */
	getcon	kcr0, r15
	ld.q	r15, TS(TS_TR0), r0
	ptabs/u	r0, tr0
	ld.q	r15, TS(TS_TR1), r0
	ptabs/u	r0, tr1
	ld.q	r15, TS(TS_R0), r0
	ld.q	r15, TS(TS_R1), r1
	ld.q	r15, TS(TS_R2), r2
	ld.q	r15, TS(TS_R3), r3
	ld.q	r15, TS(TS_R4), r4
	ld.q	r15, TS(TS_R5), r5
	ld.q	r15, TS(TS_R6), r6
	getcon	kcr1, r15		/* Back on the original stack */
	putcon	r24, usr

	/*
	 * Save state.
	 *
	 * ### Danger, Will Robinson! ###
	 *
	 * This could cause a recursive TLB miss exception inside the
	 * critical section. This is ok because the kernel stack is
	 * always wired in KSEG1, so the pmap_kernel_ipt lookup will succeed.
	 * Also, if you're wondering why we use _EXCEPTION_ENTRY here
	 * instead of jumping straight to Lsh5_vector_general, it's
	 * because that would trash the contents of tr0 ...
	 */

	_EXCEPTION_ENTRY(SZ_TRAPFRAME, CRIT_TLBMISS_TRAP)

	/* Handle it using the normal exception code. */

	pta/l	Lsh5_event_sync, tr0
	blink	tr0, r63


#ifdef PORTMASTER
/******************************************************************************
 *
 * Critical Fault Handler.
 *
 * If we take a synchronous exception while in the critical section of
 * some other event handler, we arrive here.
 *
 * This is likely to happen if the kernel stack pointer is misaligned, or
 * has gone off into orbit somewhere.
 *
 * Get onto a known good stack and save as much as possible of the machine
 * state so it can be printed to the console.
 *
 * Note: Due to the nature of this fault, not all machine state can be
 * saved. R0 is likely to be lost, and USR is definately lost..
 */
Lsh5_critical_fault:
	getcon	kcr0, r24
	st.q	r24, ES(ES_R1), r1	/* We need another register */
	or	r0, r63, r1		/* r1 == where we just came from */
	putcon	r15, kcr1		/* Save original (faulty?) stack ptr */
	LEA(_C_LABEL(sh5_panic_stack), r15)
	movi	(USPACE - SZ_TRAPFRAME), r0
	add	r15, r0, r15
	getcon	expevt, r0
	st.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0
	getcon	intevt, r0
	st.q	r15, SFO(SF_INTEVT, SZ_TRAPFRAME), r0
	getcon	tea, r0
	st.q	r15, SFO(SF_TEA, SZ_TRAPFRAME), r0
	getcon	tra, r0
	st.q	r15, SFO(SF_TRA, SZ_TRAPFRAME), r0
	getcon	spc, r0
	st.q	r15, SFO(SF_SPC, SZ_TRAPFRAME), r0
	getcon	ssr, r0
	st.q	r15, SFO(SF_SSR, SZ_TRAPFRAME), r0
	ld.q	r24, ES(ES_R0), r0
	st.q	r15, IFO(IF_R0, SZ_TRAPFRAME), r0
	ld.q	r24, ES(ES_R1), r0
	st.q	r15, IFO(IF_R1, SZ_TRAPFRAME), r0
	st.q	r15, IFO(IF_R2, SZ_TRAPFRAME), r2
	getcon	kcr1, r0
	st.q	r15, IFO(IF_R15, SZ_TRAPFRAME), r0
	ld.q	r24, ES(ES_TR0), r0
	st.q	r15, IFO(IF_TR0, SZ_TRAPFRAME), r0
	_INTR_FRAME_SAVE(SZ_TRAPFRAME)
	_TRAP_FRAME_SAVE(SZ_TRAPFRAME)

	/*
	 * Unblock exceptions.
	 * We need the TLB miss handler to be available so device
	 * registers are accessable (think console output).
	 */
	getcon	sr, r0
	ori	r0, SH5_CONREG_SR_IMASK_ALL, r0
	putcon	r0, sr
	movi	SH5_CONREG_SR_IMASK_ALL, r0
	putcon	r0, sr

	/*
	 * Fetch the ID of the current owner of the critical section, and
	 * clear the flag so that we may take further exceptions in the
	 * debugger.
	 */
	ld.q	r24, ES(ES_CRITICAL), r5
	st.q	r24, ES(ES_CRITICAL), r63

	/*
	 * Call panic_critical_fault(trapframe, exc_scratch_frame)
	 */
	LEAF(_C_LABEL(panic_critical_fault), r0)
	ptabs/l	r0, tr0
	or	r15, r63, r2	/* Pointer to trap frame */
	addi	r24, CI_ESCRATCH, r3 /* Pointer to scratch frame */
	or	r1, r63, r4	/* Where the error was detected */
	blink	tr0, r63
	/*NOTREACHED*/

#endif	/* PORTMASTER */


/******************************************************************************
 *
 * void Lcheck_softint(void)	[Note: Do not call directly from C code]
 *
 * Check if there are any pending soft interrupts, and deal with them
 * if necessary.
 */
Lcheck_softint:
	ptabs/l	r18, tr0		/* Return address to tr0 */
	LEA(_C_LABEL(ssir), r0)		/* Get bitmap of pending soft ints */
	ld.l	r0, 0, r1
	beq/l	r1, r63, tr0		/* Return if no soft ints pending */

	/*
	 * There are softints pending. The new ipl is in r2. See if it
	 * is low enough to handle any of the pending softints...
	 */

Lcheck_softint_lower:
	pta/u	1f, tr1			/* Preload the exit path */
	ori	r2, SH5_CONREG_SR_IMASK_ALL, r3
	putcon	r3, sr			/* Disable interrupts */
	shlri	r2, SH5_CONREG_SR_IMASK_SHIFT, r3
	andi	r3, SH5_CONREG_SR_IMASK_MASK, r3	/* current spl to r3 */
	ld.l	r0, 0, r1		/* Re-fetch ssir */
	movi	1, r4
	shlld	r4, r3, r4		/* r4 = 1 << curspl */
	addi	r4, -1, r4
	andc	r1, r4, r1		/* r1 &= ~((1 << curspl) - 1) */
	beq/l	r1, r63, tr1		/* Return if ipl is too high */

	/*
	 * Raise spl to the level of the highest priority pending soft int.
	 */
	nsb	r1, r3			/* r3 = ffs(r1) */
	movi	63, r1			/* Convert to usable range */
	sub	r1, r3, r3		/* r3 == target spl */
	shlli	r3, SH5_CONREG_SR_IMASK_SHIFT, r3
	movi	SH5_CONREG_SR_IMASK_ALL, r4
	andc	r2, r4, r4
	or	r4, r3, r4
	putcon	r4, sr

	/*
	 * Tail call to softintr_dispatch(oldspl(r2), softspl(r3))
	 */
	LEAF(_C_LABEL(softintr_dispatch), r0)
	ptabs/l	r0, tr0
	movi	SH5_CONREG_SR_IMASK_MASK, r0
	shlri	r2, SH5_CONREG_SR_IMASK_SHIFT, r2
	shlri	r3, SH5_CONREG_SR_IMASK_SHIFT, r3
	and	r2, r0, r2
	and	r3, r0, r3
	blink	tr0, r63

1:	putcon	r2, sr
	blink	tr0, r63



/**** C-callable procedures ****/


/******************************************************************************
 * proc_trampoline()
 *
 * This is the first bit of code executed by the child process after
 * a successful cpu_fork().
 *
 * Its purpose is simply to invoke a function somewhere in the kernel
 * (now that we're running in the child's context) before returning
 * directly to user-mode.
 *
 * cpu_fork() arranges for the following information to be passed to us:
 *
 *    r10 - The kernel function to invoke before returning to userland
 *    r11 - A single paramter for the kernel function.
 *
 * The kernel stack already has a valid trapframe, so all we need to do
 * is return through the normal trap/syscall exit path.
 */
ENTRY_NOPROFILE(proc_trampoline)
	ptabs/l	r10, tr0
	or	r11, r63, r2
	blink	tr0, r18
	LEAF(Ltrapepilogue, r0)
	ptabs/l	r0, tr0
	blink	tr0, r63


/******************************************************************************
 *
 * u_int sh5_setasid(u_int asid)
 *
 * Set the ASID bits in the Status Register to the specified value.
 * Return the previous ASID.
 */
ENTRY(sh5_setasid)
	ptabs/l	r18, tr0
	pta/l	1f, tr1
	getcon	sr, r1
	LDC32(SH5_CONREG_SR_ASID_MASK << SH5_CONREG_SR_ASID_SHIFT, r0)
	shlli	r2, SH5_CONREG_SR_ASID_SHIFT, r2 /* shift "asid" up */
	and	r2, r0, r2		/* keep only valid asid bits */
	and	r1, r0, r4		/* r4 == old asid */
	beq/l	r2, r4, tr1		/* Just return if they're the same */
	LDC32(SH5_CONREG_SR_BL | SH5_CONREG_SR_IMASK_ALL, r3)
	or	r1, r3, r3		/* SR.BL=1, IMASK=0xf */
	andc	r1, r0, r1		/* Clear current asid bits */
	gettr	tr1, r0
	putcon	r3, sr			/* Disable interrupts */
	or	r1, r2, r1		/* r1 == required SR */
	putcon	r1, ssr			/* Desired status register */
	putcon	r0, spc			/* Continuation point */
	synci
	synco
	rte
	nop
	nop
	nop
	nop
	nop
1:	shlri	r4, SH5_CONREG_SR_ASID_SHIFT, r2 /* Return previous ASID */
	blink	tr0, r63

