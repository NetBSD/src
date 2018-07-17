/* $NetBSD: copyinout.S,v 1.4 2018/07/17 10:01:59 ryo Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <aarch64/asm.h>
#include "assym.h"

RCSID("$NetBSD: copyinout.S,v 1.4 2018/07/17 10:01:59 ryo Exp $");

#define fp	x29
#define lr	x30

	.macro enter_cpu_onfault
	stp	fp, lr, [sp, #-16]!	/* save fp, lr */
	mov	fp, sp			/* link frame pointer */

	stp	x19, x20, [sp, #-16]!	/* save x19, x20 */
	mov	x19, x0			/* x19 = arg0 */
	mov	x20, x1			/* x20 = arg1 */

	/* if (cpu_set_onfault(fb) != 0) return -1 */
	sub	sp, sp, #FB_T_SIZE	/* allocate struct faultbuf */
	mov	x0, sp			/* x0 = faultbuf */
	stp	x2, x3, [sp, #-16]!	/* save x2, x3 */
	bl	cpu_set_onfault
	ldp	x2, x3, [sp], #16	/* restore x2, x3 */
	mvn	x8, xzr			/* temporary return value = -1 */
	cbnz	w0, 9f			/* return if error */

	mov	x0, x19			/* x0 = x19 = arg0 */
	mov	x1, x20			/* x1 = x20 = arg1 */

	.endm

	.macro exit_cpu_onfault
	/* curlwp->l_md.md_onfault = NULL */
	mrs	x0, tpidr_el1			/* curcpu */
	ldr	x0, [x0, #CI_CURLWP]		/* x0 = curlwp */
	str	xzr, [x0, #L_MD_ONFAULT]	/* lwp->l_md_onfault = NULL */
9:
	add	sp, sp, #FB_T_SIZE		/* pop stack */
	ldp	x19, x20, [sp], #16		/* restore x19, x20 */
	ldp	fp, lr, [sp], #16		/* restore fp, lr */
	mov	x0, x8				/* x0 = return value */
	.endm


/* LINTSTUB: int copyin(const void *uaddr, void *kaddr, size_t len); */

ENTRY(copyin)
	enter_cpu_onfault

	/*
	 * How much to copy?  Less than 32 bytes?  ...
	 */
	lsr	x4, x2, #5			/* x4 = len / 32 */
	cbz	x4, .Lcopyin_last_suboword

	/*
	 * Subtract the amount of data we copied to get a octword aligned
	 * from the length: length -= 32 - offset -> length += offset - 32;
	 */
	and	x5, x0, #31
	cbz	x5, .Lcopyin_loop

	add	x2, x2, x5
	sub	x2, x2, #32
	lsr	x4, x2, #5			/* update x4 = len / 32 */

	/* XXX: not considered alignment of destination */
	tbz	x0, #0, .Lcopyin_hword_aligned
	ldtrb	w5, [x0]
	add	x0, x0, #1
	strb	w5, [x1], #1
.Lcopyin_hword_aligned:
	tbz	x0, #1, .Lcopyin_word_aligned
	ldtrh	w5, [x0]
	add	x0, x0, #2
	strh	w5, [x1], #2
.Lcopyin_word_aligned:
	tbz	x0, #2, .Lcopyin_dword_aligned
	ldtr	w5, [x0]
	add	x0, x0, #4
	str	w5, [x1], #4
.Lcopyin_dword_aligned:
	tbz	x0, #3, .Lcopyin_qword_aligned
	ldtr	x5, [x0]
	add	x0, x0, #8
	str	x5, [x1], #8
.Lcopyin_qword_aligned:
	tbz	x0, #4, .Lcopyin_loop0
	ldtr	x5, [x0, #0]
	ldtr	x6, [x0, #8]
	add	x0, x0, #16
	stp	x5, x6, [x1], #16
.Lcopyin_loop0:
	cbz	x4, .Lcopyin_last_suboword
.Lcopyin_loop:
	ldtr	x5, [x0, #0]
	ldtr	x6, [x0, #8]
	ldtr	x7, [x0, #16]
	ldtr	x8, [x0, #24]
	add	x0, x0, #32
	stp	x5, x6, [x1], #16
	stp	x7, x8, [x1], #16
	sub	x4, x4, #1
	cbnz	x4, .Lcopyin_loop
.Lcopyin_last_suboword:
	tbz	x2, #4, .Lcopyin_last_subqword
	ldtr	x5, [x0, #0]
	ldtr	x6, [x0, #8]
	add	x0, x0, #16
	stp	x5, x6, [x1], #16
.Lcopyin_last_subqword:
	tbz	x2, #3, .Lcopyin_last_subdword
	ldtr	x5, [x0]
	add	x0, x0, #8
	str	x5, [x1], #8
.Lcopyin_last_subdword:
	tbz	x2, #2, .Lcopyin_last_subword
	ldtr	w5, [x0]
	add	x0, x0, #4
	str	w5, [x1], #4
.Lcopyin_last_subword:
	tbz	x2, #1, .Lcopyin_last_subhword
	ldtrh	w5, [x0]
	add	x0, x0, #2
	strh	w5, [x1], #2
.Lcopyin_last_subhword:
	tbz	x2, #0, .Lcopyin_done
	ldtrb	w5, [x0]
	strb	w5, [x1]
.Lcopyin_done:
	mov	x8, #0

	exit_cpu_onfault
	ret
END(copyin)


/* LINTSTUB: int copyout(const void *kaddr, void *uaddr, size_t len); */

ENTRY(copyout)
	enter_cpu_onfault

	/*
	 * How much to copy?  Less than 32 bytes?  ...
	 */
	lsr	x4, x2, #5			/* x4 = len / 32 */
	cbz	x4, .Lcopyout_last_suboword

	/*
	 * Subtract the amount of data we copied to get a octword aligned
	 * from the length: length -= 32 - offset -> length += offset - 32;
	 */
	and	x5, x0, #31
	cbz	x5, .Lcopyout_loop

	add	x2, x2, x5
	sub	x2, x2, #32
	lsr	x4, x2, #5			/* update x4 = len / 32 */

	/* XXX: not considered alignment of destination */
	tbz	x0, #0, .Lcopyout_hword_aligned
	ldrb	w5, [x0], #1
	sttrb	w5, [x1]
	add	x1, x1, #1
.Lcopyout_hword_aligned:
	tbz	x0, #1, .Lcopyout_word_aligned
	ldrh	w5, [x0], #2
	sttrh	w5, [x1]
	add	x1, x1, #2
.Lcopyout_word_aligned:
	tbz	x0, #2, .Lcopyout_dword_aligned
	ldr	w5, [x0], #4
	sttr	w5, [x1]
	add	x1, x1, #4
.Lcopyout_dword_aligned:
	tbz	x0, #3, .Lcopyout_qword_aligned
	ldr	x5, [x0], #8
	sttr	x5, [x1]
	add	x1, x1, #8
.Lcopyout_qword_aligned:
	tbz	x0, #4, .Lcopyout_loop0
	ldp	x5, x6, [x0], #16
	sttr	x5, [x1, #0]
	sttr	x6, [x1, #8]
	add	x1, x1, #16
.Lcopyout_loop0:
	cbz	x4, .Lcopyout_last_suboword
.Lcopyout_loop:
	ldp	x5, x6, [x0], #16
	ldp	x7, x8, [x0], #16
	sttr	x5, [x1, #0]
	sttr	x6, [x1, #8]
	sttr	x7, [x1, #16]
	sttr	x8, [x1, #24]
	add	x1, x1, #32
	sub	x4, x4, #1
	cbnz	x4, .Lcopyout_loop
.Lcopyout_last_suboword:
	tbz	x2, #4, .Lcopyout_last_subqword
	ldp	x5, x6, [x0], #16
	sttr	x5, [x1, #0]
	sttr	x6, [x1, #8]
	add	x1, x1, #16
.Lcopyout_last_subqword:
	tbz	x2, #3, .Lcopyout_last_subdword
	ldr	x5, [x0], #8
	sttr	x5, [x1]
	add	x1, x1, #8
.Lcopyout_last_subdword:
	tbz	x2, #2, .Lcopyout_last_subword
	ldr	w5, [x0], #4
	sttr	w5, [x1]
	add	x1, x1, #4
.Lcopyout_last_subword:
	tbz	x2, #1, .Lcopyout_last_subhword
	ldrh	w5, [x0], #2
	sttrh	w5, [x1]
	add	x1, x1, #2
.Lcopyout_last_subhword:
	tbz	x2, #0, .Lcopyout_done
	ldrb	w5, [x0]
	sttrb	w5, [x1]
.Lcopyout_done:
	mov	x8, #0

	exit_cpu_onfault
	ret
END(copyout)


/* LINTSTUB: int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done); */

ENTRY(copyinstr)
	enter_cpu_onfault

	mov	x4, xzr			/* i = 0 */
	cbz	x2, copyinstr_done	/* if (len == 0) goto done */
copyinstr_loop:
	ldtrb	w5, [x0]		/* ch = *uaddr++ */
	add	x0, x0, #1
	strb	w5, [x1], #1		/* *kaddr++ = ch */
	add	x4, x4, #1		/* i++ */
	cbz	w5, copyinstr_done	/* if (ch == '\0') goto done */

	cmp	x4, x2			/* if (i < len) goto loop */
	bcc	copyinstr_loop

copyinstr_done:
	cbz	x3, 1f			/* if (done != NULL) *done = i */
	str	x4, [x3]
1:

	mov	x8, #0			/* return 0 */

	exit_cpu_onfault
	ret
END(copyinstr)


/* LINTSTUB: int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done); */

ENTRY(copyoutstr)
	enter_cpu_onfault

	mov	x4, xzr			/* i = 0 */
	cbz	x2, copyoutstr_done	/* if (len == 0) goto done */
copyoutstr_loop:
	ldrb	w5, [x0], #1		/* ch = *kaddr++ */
	sttrb	w5, [x1]		/* *uaddr++ = ch */
	add	x1, x1, #1
	add	x4, x4, #1		/* i++ */
	cbz	w5, copyoutstr_done	/* if (ch == '\0') goto done */

	cmp	x4, x2			/* if (i < len) goto loop */
	bcc	copyoutstr_loop

copyoutstr_done:
	cbz	x3, 1f			/* if (done != NULL) *done = i */
	str	x4, [x3]
1:

	mov	x8, #0			/* return 0 */

	exit_cpu_onfault
	ret
END(copyoutstr)
