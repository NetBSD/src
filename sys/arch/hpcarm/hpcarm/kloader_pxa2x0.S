/*	$NetBSD: kloader_pxa2x0.S,v 1.1.4.2 2012/04/17 00:06:24 yamt Exp $	*/

/*-
 * Copyright (C) 2012 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#include <arm/armreg.h>

/*
 * CPWAIT -- Canonical method to wait for CP15 update.
 * NOTE: Clobbers the specified temp reg.
 * copied from arm/arm/cpufunc_asm_xscale.S
 * XXX: better be in a common header file.
 */
#define	CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define	CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0	/* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */

#define	VATOPA(reg)							\
	bic	reg, reg, #0xff000000	/* clear upper 8 bits */	;\
	orr	reg, reg, #0xa0000000	/* OR in physical base address */

	.text
	.align	0
/*
 * void
 * kloader_pxa2x0_boot(struct kloader_bootinfo *kbi,
 *                     struct kloader_page_tag *tag)
 *
 * r0: kbi, r1: tag
 */
	.global	_C_LABEL(kloader_pxa2x0_boot)
_C_LABEL(kloader_pxa2x0_boot):
	mov	lr, r0			/* lr: kbi */
	VATOPA(lr)

	/* We will go ahead and disable the MMU here. */
	adr	r8, .Lmmuoff
	VATOPA(r8)

	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	bic	r2, r2, #CPU_CONTROL_DC_ENABLE
	bic	r2, r2, #CPU_CONTROL_IC_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	mov	pc, r8			/* Heave-ho! */

	.align	0
.Lmmuoff:
	nop
	nop
	nop

	/* Load kernel image */
	mov	r4, r1		/* tag */
1:	mov	r3, r4
	cmp	r3, #0		/* tag == NULL */
	beq	3f
	VATOPA(r3)
	ldmia	r3, {r4-r7}	/* r4: next, r5: src, r6: dst, r7: sz */
	VATOPA(r5)
	VATOPA(r6)
	add	r8, r6, r7	/* r8: dst + sz */
2:	ldr	r9, [r5], #4	/* copy */
	str	r9, [r6], #4
	cmp	r8, r6
	bne	2b
	b	1b

3:
	/* Set temporary stack pointer */
	ldr	sp, .Lstack_addr

	/* Jump to kernel entry */
	mov	r3, lr			/* r3: kbi */
	ldr	lr, [r3]		/* kbi->entry */
	VATOPA(lr)
	ldr	r0, [r3, #4]		/* kbi->argc */
	ldr	r1, [r3, #8]		/* kbi->argv */
	add	r2, r3, #12		/* &kbi->bootinfo */

	/* Fixup argv[] */
	cmp	r1, #0			/* argv == NULL */
	beq	5f
	VATOPA(r1)
	cmp	r0, #0			/* argc == 0 */
	beq	5f

	mov	r3, r0
	mov	r4, r1
4:	ldr	r5, [r4]
	VATOPA(r5)
	str	r5, [r4], #4
	subs	r3, r3, #1
	bgt	4b

5:	/* Jump! */
	mov	pc, lr

99:	b	99b

.Lstack_addr:
	.word	0xa0004000
