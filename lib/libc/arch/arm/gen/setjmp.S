/*	$NetBSD: setjmp.S,v 1.8.10.1 2007/08/28 17:36:30 matt Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <machine/setjmp.h>

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from the last call to
 *	setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is restored.
 */

ENTRY(__setjmp14)
#ifdef __thumb__
	/* Get the signal mask. */
	push	{r0, r4-r7, lr}
	add	r0, r0, #(_JB_SIGMASK * 4)
	mov	r2, r0
	mov	r1, #0
	mov	r0, #0
	bl	PLT_SYM(_C_LABEL(__sigprocmask14))
	pop	{r0}

	ldr	r2, .Lsetjmp_magic
	str	r2, [r0]

	add	r0, r0, #56	/* skip magic and floating point */

	/* Store integer registers */
        stmia	r0!, {r4-r7}

	mov	r4, r8
	mov	r5, r9
	mov	r6, r10
	mov	r7, r11
        stmia	r0!, {r4-r7}

	mov	r4, r12
	mov	r5, r13
	mov	r6, r14
        stmia	r0!, {r4-r6}

        mov	r0, #0
        pop	{r4-r7, pc}
#else
	/* Get the signal mask. */
	stmfd	sp!, {r0-r2, r14}
	add	r2, r0, #(_JB_SIGMASK * 4)
	mov	r1, #0x00000000
	mov	r0, #0x00000000
	bl	PLT_SYM(_C_LABEL(__sigprocmask14))
	ldmfd	sp!, {r0-r2, r14}

	ldr	r1, .Lsetjmp_magic
	str	r1, [r0]

#ifdef SOFTFLOAT
	add	r0, r0, #52
#else
	/* Store fp registers */
	sfm	f4, 4, [r0], #48
	/* Store fpsr */
	rfs	r1
	str	r1, [r0], #0x0004
#endif	/*SOFTFLOAT*/
	/* Store integer registers */
        stmia	r0, {r4-r14}
        mov	r0, #0x00000000
        RET
#endif /* __thumb__ */

ENTRY(__longjmp14)
#ifdef __thumb__
	ldr	r2, .Lsetjmp_magic
	ldr	r3, [r0]
	cmp	r2, r3
	bne	.Lbotch

	/* Restore the signal mask. */
	push	{r0, lr}
	mov	r2, #0
	add	r0, r0, #(_JB_SIGMASK * 4)
	mov	r1, r0
	mov	r0, #3				/* SIG_SETMASK */
	bl	PLT_SYM(_C_LABEL(__sigprocmask14))
	pop	{r0}

	add	r0, r0, #56
	/* Restore integer registers */
        ldmia	r0!, {r4-r7}
	push	{r4-r7}

        ldmia	r0!, {r4-r7}
	mov	r8, r4
	mov	r9, r5
	mov	r10, r6
	mov	r11, r7

        ldmia	r0!, {r4-r6}
	mov	r12, r4
	mov	sp, r5
	mov	lr, r6

	/* Validate sp and lr */
	cmp	r5, #0		/* sp */
	beq	.Lbotch
	cmp	r6, #0		/* lr */
	beq	.Lbotch

	/* Set return value */

	mov	r0, r1
	bne	1f
	mov	r0, #1
1:	pop	{r4-r7, pc}
#else
	ldr	r2, .Lsetjmp_magic
	ldr	r3, [r0]
	teq	r2, r3
	bne	.Lbotch

	/* Restore the signal mask. */
	stmfd	sp!, {r0-r2, r14}
	mov	r2, #0x00000000
	add	r1, r0, #(_JB_SIGMASK * 4)
	mov	r0, #3				/* SIG_SETMASK */
	bl	PLT_SYM(_C_LABEL(__sigprocmask14))
	ldmfd	sp!, {r0-r2, r14}

	add	r0, r0, #4
#ifdef SOFTFLOAT
	add	r0, r0, #52
#else
	/* Restore fp registers */
	lfm	f4, 4, [r0], #48
	/* Restore FPSR */
	ldr	r4, [r0], #0x0004
	wfs	r4
#endif	/* SOFTFLOAT */
	/* Restore integer registers */
        ldmia	r0, {r4-r14}

	/* Validate sp and r14 */
	teq	sp, #0
	teqne	r14, #0
	beq	.Lbotch

	/* Set return value */

	mov	r0, r1
	teq	r0, #0x00000000
	moveq	r0, #0x00000001
	RET
#endif

	/* validation failed, die die die. */
.Lbotch:
	bl	PLT_SYM(_C_LABEL(longjmperror))
	bl	PLT_SYM(_C_LABEL(abort))
	b	. - 8		/* Cannot get here */

	.p2align 2
.Lsetjmp_magic:
	.word	_JB_MAGIC_SETJMP
