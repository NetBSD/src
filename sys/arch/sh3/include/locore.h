/*	$NetBSD: locore.h,v 1.1 2002/02/24 18:19:42 uch Exp $	*/

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

/* XXX XXX XXX */
#define SH3_BBRA	0xffffffb8
#define SH4_BBRA	0xff200008
#define SH3_EXPEVT	0xffffffd4
#define SH3_INTEVT	0xffffffd8
#define SH4_EXPEVT	0xff000024	
#define SH4_INTEVT	0xff000028
/* XXX XXX XXX */

#if defined(SH3) && defined(SH4)
#define MOV(x, r)	mov.l _L./**/x, r; mov.l @r, r
#define	REG_SYMBOL(x)	_L./**/x:	.long	_C_LABEL(__sh_/**/x)
#define	FUNC_SYMBOL(x)	_L./**/x:	.long	_C_LABEL(__sh_/**/x)	
#elif defined(SH3)
#define MOV(x, r)	mov.l _L./**/x, r
#define	REG_SYMBOL(x)	_L./**/x:	.long	SH3_/**/x
#define	FUNC_SYMBOL(x)	_L./**/x:	.long	_C_LABEL(sh3_/**/x)
#elif defined(SH4)	
#define MOV(x, r)	mov.l _L./**/x, r
#define	REG_SYMBOL(x)	_L./**/x:	.long	SH4_/**/x
#define	FUNC_SYMBOL(x)	_L./**/x:	.long	_C_LABEL(sh4_/**/x)
#endif

/*
 * BANK1 r7 contains kernel stack top address.
 */		 		 	
/*
 * EXCEPTION_ENTRY:		
 *	+ setup stack pointer
 *	+ save all register to stack. (struct trapframe)
 *	+ change bank from 1 to 0
 *	+ set BANK0 (r4, r5) = (ssr, spc)
 */
#define	EXCEPTION_ENTRY							;\
	/* Check kernel/user mode. */					;\
	mov	#0x40,	r3						;\
	swap.b	r3,	r3						;\
	stc	ssr,	r1						;\
	swap.w	r3,	r3	/* r3 = 0x40000000 */			;\
	mov	r1,	r0	/* r1 = r0 = SSR */			;\
	and	r3,	r0						;\
	tst	r0,	r0	/* if (SSR.MD == 0) T = 1 */		;\
	bf/s	1f		/* T==0 ...Exception from kernel mode */;\
	 mov	r15,	r0	/* r0 = old stack */			;\
	/* Exception from user mode */					;\
	mov	r7,	r15	/* change to kernel stack */		;\
1:									;\
	/* Save registers */						;\
	mov.l	r0,	@-r15	/* tf_r15 */				;\
	stc.l	r0_bank,@-r15	/* tf_r0  */				;\
	stc.l	r1_bank,@-r15	/* tf_r1  */				;\
	stc.l	r2_bank,@-r15	/* tf_r2  */				;\
	stc.l	r3_bank,@-r15	/* tf_r3  */				;\
	stc.l	r4_bank,@-r15	/* tf_r4  */				;\
	stc.l	r5_bank,@-r15	/* tf_r5  */				;\
	stc.l	r6_bank,@-r15	/* tf_r6  */				;\
	stc.l	r7_bank,@-r15	/* tf_r7  */				;\
	mov.l	r8,	@-r15	/* tf_r8  */				;\
	mov.l	r9,	@-r15	/* tf_r9  */				;\
	mov.l	r10,	@-r15	/* tf_r10 */				;\
	mov.l	r11,	@-r15	/* tf_r11 */				;\
	mov.l	r12,	@-r15	/* tf_r12 */				;\
	mov.l	r13,	@-r15	/* tf_r13 */				;\
	mov.l	r14,	@-r15	/* tf_r14 */				;\
	sts.l	pr,	@-r15	/* tf_pr  */				;\
	sts.l	mach,	@-r15	/* tf_mach*/				;\
	sts.l	macl,	@-r15	/* tf_macl*/				;\
	mov.l	r1,	@-r15	/* tf_ssr */				;\
	stc.l	spc,	@-r15	/* tf_spc */				;\
	add	#-8,	r15	/* skip tf_ubc, tf_trapno */		;\
	/* Change register bank to 0 */					;\
	shlr	r3		/* r3 = 0x20000000 */			;\
	stc	sr,	r0	/* r0 = SR */				;\
	not	r3,	r3						;\
	and	r0,	r3						;\
	ldc	r3,	sr	/* SR.RB = 0 */				;\
	/* Set up argument. r4 = ssr, r5 = spc */			;\
	stc	r1_bank,r4						;\
	stc	spc,	r5

/*	
 * EXCEPTION_RETURN:	 
 *	+ block exception	
 *	+ restore all register from stack. 
 *	+ rte.	
 */
#define	EXCEPTION_RETURN						;\
	mov	#0x10,	r0						;\
	swap.b	r0,	r0						;\
	swap.w	r0,	r0	/* r0 = 0x10000000 */			;\
	stc	sr,	r1						;\
	or	r0,	r1						;\
	ldc	r1,	sr	/* SR.BL = 1 */				;\
	add	#8,	r15	/* skip tf_trapno, tf_ubc */		;\
	mov.l	@r15+,	r0	/* tf_spc */				;\
	ldc	r0,	spc						;\
	mov.l	@r15+,	r0	/* tf_ssr */				;\
	ldc	r0,	ssr						;\
	lds.l	@r15+,	macl	/* tf_macl*/				;\
	lds.l	@r15+,	mach	/* tf_mach*/				;\
	lds.l	@r15+,	pr	/* tf_pr  */				;\
	mov.l	@r15+,	r14	/* tf_r14 */				;\
	mov.l	@r15+,	r13	/* tf_r13 */				;\
	mov.l	@r15+,	r12	/* tf_r12 */				;\
	mov.l	@r15+,	r11	/* tf_r11 */				;\
	mov.l	@r15+,	r10	/* tf_r10 */				;\
	mov.l	@r15+,	r9	/* tf_r9  */				;\
	mov.l	@r15+,	r8	/* tf_r8  */				;\
	mov.l	@r15+,	r7	/* tf_r7  */				;\
	mov.l	@r15+,	r6	/* tf_r6  */				;\
	mov.l	@r15+,	r5	/* tf_r5  */				;\
	mov.l	@r15+,	r4	/* tf_r4  */				;\
	mov.l	@r15+,	r3	/* tf_r3  */				;\
	mov.l	@r15+,	r2	/* tf_r2  */				;\
	mov.l	@r15+,	r1	/* tf_r1  */				;\
	mov.l	@r15+,	r0	/* tf_r0  */				;\
	mov.l	@r15,	r15	/* tf_r15 */				;\
	rte								;\
	nop


/*
 * Macros to disable and enable exceptions (including interrupts).
 * This modifies SR.BL
 */
#define __EXCEPTION_BLOCK_r0_r1						;\
	mov	#0x10,	r0						;\
	swap.b	r0,	r0						;\
	swap.w	r0,	r0	/* r0 = 0x10000000 */			;\
	stc	sr,	r1						;\
	or	r0,	r1						;\
	ldc	r1,	sr	/* block exceptions */

#define __EXCEPTION_UNBLOCK_r0_r1					;\
	mov	#0x10,	r0						;\
	swap.b	r0,	r0						;\
	swap.w	r0,	r0	/* r0 = 0x10000000 */			;\
	not	r0,	r0						;\
	stc	sr,	r1						;\
	and	r0,	r1						;\
	ldc	r1,	sr	/* unblock exceptions */

/*
 * Macros to disable and enable interrupts.
 * This modifies SR.I[0-3]
 */
#define	__INTR_MASK_r0_r1						;\
	mov	#0x78,	r0						;\
	shll	r0		/* r0 = 0x000000f0 */			;\
	stc	sr,	r1						;\
	or	r0,	r1						;\
	ldc	r1,	sr	/* mask all interrupt */

#define __INTR_UNMASK_r0_r1						;\
	mov	#0x78,	r0						;\
	shll	r0		/* r0 = 0x000000f0 */			;\
	not	r0,	r0						;\
	stc	sr,	r1						;\
	and	r0,	r1						;\
	ldc	r1,	sr	/* unmask all interrupt */


#define	RECURSEENTRY							;\
	mov	r15,	r0						;\
	mov.l	r0,	@-r15						;\
	mov.l	r0,	@-r15						;\
	mov.l	r1,	@-r15						;\
	mov.l	r2,	@-r15						;\
	mov.l	r3,	@-r15						;\
	mov.l	r4,	@-r15						;\
	mov.l	r5,	@-r15						;\
	mov.l	r6,	@-r15						;\
	mov.l	r7,	@-r15						;\
	mov.l	r8,	@-r15						;\
	mov.l	r9,	@-r15						;\
	mov.l	r10,	@-r15						;\
	mov.l	r11,	@-r15						;\
	mov.l	r12,	@-r15						;\
	mov.l	r13,	@-r15						;\
	mov.l	r14,	@-r15						;\
	sts.l	pr,	@-r15						;\
	sts.l	mach,	@-r15						;\
	sts.l	macl,	@-r15						;\
	stc.l	ssr,	@-r15						;\
	stc.l	spc,	@-r15						;\
	add	#-8, r15	/* tf_ubc, tf_trapno */
