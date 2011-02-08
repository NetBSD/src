/*	$NetBSD: locore.h,v 1.20.2.1 2011/02/08 16:19:39 bouyer Exp $	*/

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

#ifdef _LOCORE

#ifdef __STDC__
#if defined(SH3) && defined(SH4)
#define	MOV(x, r)	mov.l .L_ ## x, r; mov.l @r, r
#define	REG_SYMBOL(x)	.L_ ## x:	.long	_C_LABEL(__sh_ ## x)
#define	FUNC_SYMBOL(x)	.L_ ## x:	.long	_C_LABEL(__sh_ ## x)
#elif defined(SH3)
#define	MOV(x, r)	mov.l .L_ ## x, r
#define	REG_SYMBOL(x)	.L_ ## x:	.long	SH3_ ## x
#define	FUNC_SYMBOL(x)	.L_ ## x:	.long	_C_LABEL(sh3_ ## x)
#elif defined(SH4)
#define	MOV(x, r)	mov.l .L_ ## x, r
#define	REG_SYMBOL(x)	.L_ ## x:	.long	SH4_ ## x
#define	FUNC_SYMBOL(x)	.L_ ## x:	.long	_C_LABEL(sh4_ ## x)
#endif /* SH3 && SH4 */
#else /* !__STDC__ */
#if defined(SH3) && defined(SH4)
#define	MOV(x, r)	mov.l .L_/**/x, r; mov.l @r, r
#define	REG_SYMBOL(x)	.L_/**/x:	.long	_C_LABEL(__sh_/**/x)
#define	FUNC_SYMBOL(x)	.L_/**/x:	.long	_C_LABEL(__sh_/**/x)
#elif defined(SH3)
#define	MOV(x, r)	mov.l .L_/**/x, r
#define	REG_SYMBOL(x)	.L_/**/x:	.long	SH3_/**/x
#define	FUNC_SYMBOL(x)	.L_/**/x:	.long	_C_LABEL(sh3_/**/x)
#elif defined(SH4)
#define	MOV(x, r)	mov.l .L_/**/x, r
#define	REG_SYMBOL(x)	.L_/**/x:	.long	SH4_/**/x
#define	FUNC_SYMBOL(x)	.L_/**/x:	.long	_C_LABEL(sh4_/**/x)
#endif /* SH3 && SH4 */
#endif /* __STDC__ */

/*
 * BANK1 r6 contains current trapframe pointer.
 * BANK1 r7 contains bottom address of lwp's kernel stack.
 */
/*
 * __EXCEPTION_ENTRY:
 *	+ setup stack pointer
 *	+ save all registers to trapframe.
 *	+ setup kernel stack.
 *	+ change bank from 1 to 0
 *	+ NB: interrupt vector "knows" that r0_bank1 = ssp
 */
#define	__EXCEPTION_ENTRY						;\
	/* Check kernel/user mode. */					;\
	mov	#0x40,	r3						;\
	stc	ssr,	r2	/* r2 = SSR */				;\
	swap.b	r3,	r3						;\
	mov	r14,	r1						;\
	swap.w	r3,	r3	/* r3 = PSL_MD */			;\
	mov	r6,	r14	/* trapframe pointer */			;\
	tst	r3,	r2	/* if (SSR.MD == 0) T = 1 */		;\
	mov.l	r1,	@-r14	/* save tf_r14 */			;\
	bf/s	1f		/* T==0 ...Exception from kernel mode */;\
	 mov	r15,	r0						;\
	/* Exception from user mode */					;\
	mov	r7,	r15	/* change to kernel stack */		;\
1:									;\
	/* Save remaining registers */					;\
	mov.l	r0,	@-r14	/* tf_r15 */				;\
	stc.l	r0_bank,@-r14	/* tf_r0  */				;\
	stc.l	r1_bank,@-r14	/* tf_r1  */				;\
	stc.l	r2_bank,@-r14	/* tf_r2  */				;\
	stc.l	r3_bank,@-r14	/* tf_r3  */				;\
	stc.l	r4_bank,@-r14	/* tf_r4  */				;\
	stc.l	r5_bank,@-r14	/* tf_r5  */				;\
	stc.l	r6_bank,@-r14	/* tf_r6  */				;\
	stc.l	r7_bank,@-r14	/* tf_r7  */				;\
	mov.l	r8,	@-r14	/* tf_r8  */				;\
	mov.l	r9,	@-r14	/* tf_r9  */				;\
	mov.l	r10,	@-r14	/* tf_r10 */				;\
	mov.l	r11,	@-r14	/* tf_r11 */				;\
	mov.l	r12,	@-r14	/* tf_r12 */				;\
	mov.l	r13,	@-r14	/* tf_r13 */				;\
	sts.l	pr,	@-r14	/* tf_pr  */				;\
	sts.l	mach,	@-r14	/* tf_mach*/				;\
	sts.l	macl,	@-r14	/* tf_macl*/				;\
	stc.l	gbr,	@-r14	/* tf_gbr */				;\
	mov.l	r2,	@-r14	/* tf_ssr */				;\
	stc.l	spc,	@-r14	/* tf_spc */				;\
	add	#-8,	r14	/* skip tf_ubc, tf_expevt */		;\
	mov	r14,	r6	/* store frame pointer */		;\
	/* Change register bank to 0 */					;\
	shlr	r3		/* r3 = PSL_RB */			;\
	stc	sr,	r1	/* r1 = SR */				;\
	not	r3,	r3						;\
	and	r1,	r3						;\
	ldc	r3,	sr	/* SR.RB = 0 */


/*
 * __EXCEPTION_RETURN:
 *	+ block exceptions
 *	+ restore all registers from stack.
 *	+ rte.
 */
#define	__EXCEPTION_RETURN						;\
	mov	#0x10,	r0						;\
	swap.b	r0,	r0						;\
	swap.w	r0,	r0	/* r0 = 0x10000000 */			;\
	stc	sr,	r1						;\
	or	r0,	r1						;\
	ldc	r1,	sr	/* SR.BL = 1 */				;\
	stc	r6_bank,r0						;\
	mov	r0,	r14						;\
	add	#TF_SIZE, r0						;\
	ldc	r0,	r6_bank	/* roll up frame pointer */		;\
	add	#8,	r14	/* skip tf_expevt, tf_ubc */		;\
	ldc.l	@r14+,	spc	/* tf_spc */				;\
	ldc.l	@r14+,	ssr	/* tf_ssr */				;\
	ldc.l	@r14+,	gbr	/* tf_gbr */				;\
	lds.l	@r14+,	macl	/* tf_macl*/				;\
	lds.l	@r14+,	mach	/* tf_mach*/				;\
	lds.l	@r14+,	pr	/* tf_pr  */				;\
	mov.l	@r14+,	r13	/* tf_r13 */				;\
	mov.l	@r14+,	r12	/* tf_r12 */				;\
	mov.l	@r14+,	r11	/* tf_r11 */				;\
	mov.l	@r14+,	r10	/* tf_r10 */				;\
	mov.l	@r14+,	r9	/* tf_r9  */				;\
	mov.l	@r14+,	r8	/* tf_r8  */				;\
	mov.l	@r14+,	r7	/* tf_r7  */				;\
	mov.l	@r14+,	r6	/* tf_r6  */				;\
	mov.l	@r14+,	r5	/* tf_r5  */				;\
	mov.l	@r14+,	r4	/* tf_r4  */				;\
	mov.l	@r14+,	r3	/* tf_r3  */				;\
	mov.l	@r14+,	r2	/* tf_r2  */				;\
	mov.l	@r14+,	r1	/* tf_r1  */				;\
	mov.l	@r14+,	r0	/* tf_r0  */				;\
	mov.l	@r14+	r15	/* tf_r15 */				;\
	mov.l	@r14+,	r14	/* tf_r14 */				;\
	rte								;\
	 nop


/*
 * Macros to disable and enable exceptions (including interrupts).
 * This modifies SR.BL
 */

#define	__EXCEPTION_BLOCK(Rn, Rm)					;\
	mov	#0x10,	Rn						;\
	swap.b	Rn,	Rn						;\
	swap.w	Rn,	Rn	/* Rn = 0x10000000 */			;\
	stc	sr,	Rm						;\
	or	Rm,	Rn						;\
	ldc	Rn,	sr	/* block exceptions */

#define	__EXCEPTION_UNBLOCK(Rn, Rm)					;\
	mov	#0xef,	Rn	/* ~0x10 */				;\
	swap.b	Rn,	Rn						;\
	swap.w	Rn,	Rn	/* Rn = ~0x10000000 */			;\
	stc	sr,	Rm						;\
	and	Rn,	Rm						;\
	ldc	Rm,	sr	/* unblock exceptions */

/*
 * Macros to disable and enable interrupts.
 * This modifies SR.I[0-3]
 */
#define	__INTR_MASK(Rn, Rm)						;\
	mov	#0x78,	Rn						;\
	shll	Rn		/* Rn = 0x000000f0 */			;\
	stc	sr,	Rm						;\
	or	Rn,	Rm						;\
	ldc	Rm,	sr	/* mask all interrupts */

#define	__INTR_UNMASK(Rn, Rm)						;\
	mov	#0x78,	Rn						;\
	shll	Rn		/* Rn = 0x000000f0 */			;\
	not	Rn,	Rn						;\
	stc	sr,	Rm						;\
	and	Rn,	Rm						;\
	ldc	Rm,	sr	/* unmask all interrupts */


/*
 * Since __INTR_MASK + __EXCEPTION_UNBLOCK is common sequence, provide
 * this combo version that does stc/ldc just once.
 */
#define __INTR_MASK_EXCEPTION_UNBLOCK(Rs, Ri, Rb)			 \
	mov	#0x78, Ri	/* 0xf0 >> 1 */				;\
	mov	#0xef, Rb	/* ~0x10 */				;\
	shll	Ri		/* Ri = PSL_IMASK */			;\
	swap.b	Rb, Rb							;\
	stc	sr, Rs							;\
	swap.w	Rb, Rb		/* Rb = ~PSL_BL */			;\
	or	Ri, Rs		/* SR |= PSL_IMASK */			;\
	and	Rb, Rs		/* SR &= ~PSL_BL */			;\
	ldc	Rs, sr


#else /* !_LOCORE */

void sh3_switch_setup(struct lwp *);
void sh4_switch_setup(struct lwp *);
void sh3_switch_resume(struct lwp *);
void sh4_switch_resume(struct lwp *);
extern void (*__sh_switch_resume)(struct lwp *);

#endif /* !_LOCORE */
