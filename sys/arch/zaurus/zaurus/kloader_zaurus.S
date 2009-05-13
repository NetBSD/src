/*	$NetBSD: kloader_zaurus.S,v 1.1.6.2 2009/05/13 17:18:55 jym Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 */

#include <machine/asm.h>

#include <arm/armreg.h>
#include <arm/arm32/pte.h>
#include <arm/arm32/pmap.h>		/* for PMAP_DOMAIN_KERNEL */

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

	.text
/*
 * void
 * kloader_zaurus_boot(struct kloader_bootinfo *kbi,
 *                     struct kloader_page_tag *tag)
 *
 * r0: kbi, r1: tag
 */
	.global	_C_LABEL(kloader_zaurus_boot)
_C_LABEL(kloader_zaurus_boot):
	mov	r0, r12			/* r12: kbi */
	bic	r12, r12, #0xff000000
	orr	r12, r12, #0xa0000000

	/* We will go ahead and disable the MMU here. */
	adr	r8, .Lmmuoff
	bic	r8, r8, #0xff000000	/* clear upper 8 bits */
	orr	r8, r8, #0xa0000000	/* OR in physical base address */

	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	bic	r2, r2, #CPU_CONTROL_DC_ENABLE
	bic	r2, r2, #CPU_CONTROL_IC_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	mov	pc, r8			/* Heave-ho! */

.Lmmuoff:

	/* Load kernel image */
	mov	r4, r1		/* tag */
1:	mov	r3, r4
	cmp	r3, #0		/* tag == NULL */
	beq	3f
	bic	r3, r3, #0xff000000
	orr	r3, r3, #0xa0000000
	ldmia	r3, {r4-r7}	/* r4: next, r5: src, r6: dst, r7: sz */
	bic	r5, r5, #0xff000000
	orr	r5, r5, #0xa0000000
	bic	r6, r6, #0xff000000
	orr	r6, r6, #0xa0000000
	add	r8, r6, r7	/* r8: dst + sz */
2:	ldr	r9, [r5], #4	/* copy */
	str	r9, [r6], #4
	cmp	r8, r6
	bne	2b
#ifdef PROGRESS
	bl	.Lprogress
#endif
	b	1b
3:
#ifdef PROGRESS
	bl	.Lputcrlf
#endif

	/* set temporary stack pointer */
	ldr	sp, .Lstack_addr

	/* Jump to kernel entry */
#if 0	/* XXX */
	ldr	r0, [r12]	/* kbi->entry */
#else
	ldr	r0, .Lstart_addr
#endif
	mov	pc, r0

99:	b	99b

.Lstack_addr:
	.word	0xa0004000

.Lstart_addr:
	.word	0xa0200000

#ifdef PROGRESS
.Lprogress:
	mov	r0, #0x2e /*'.'*/
	b	.Lputchar

.Lputchar:
	ldr	r2, .Lffuart_txdata
	ldr	r1, .Lffuart_txbusy
1:
	ldr	r9, [r1]
	tst	r9, #0x20
	beq	1b
	str	r0, [r2]
	mov	pc, lr

.Lhexprint:
	ldr	r2, .Lffuart_txdata
	ldr	r1, .Lffuart_txbusy
	mov	r10, #32
1:	sub	r10, r10, #4
	mov	r11, r0, lsr r10
	and	r11, r11, #0xf
2:	ldr	r9, [r1]
	tst	r9, #0x20
	beq	2b
	adr	r12, .Lhextable
	ldrb	r9, [r12, r11]
	str	r9, [r2]
	cmp	r10, #0
	bne	1b
3:	ldr	r9, [r1]
	tst	r9, #0x20
	beq	3b
	mov	r9, #0x0d
	str	r9, [r2]
4:	ldr	r9, [r1]
	tst	r9, #0x20
	beq	4b
	mov	r9, #0x0a
	str	r9, [r2]
	mov	pc, lr

.Lputcrlf:
	ldr	r2, .Lffuart_txdata
	ldr	r1, .Lffuart_txbusy
	b	3b

	.align 2

.Lffuart_txdata:
	.word	0x40100000
.Lffuart_txbusy:
	.word	0x40100014

.Lhextable:
	.asciz	"0123456789abcdef"
#endif
