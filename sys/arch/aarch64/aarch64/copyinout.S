/* $NetBSD: copyinout.S,v 1.1 2014/08/10 05:47:37 matt Exp $ */

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

RCSID("$NetBSD: copyinout.S,v 1.1 2014/08/10 05:47:37 matt Exp $");

/* LINTSTUB: int copyin(const void *uaddr, void *kaddr, size_t len); */

ENTRY(copyin)
	mrs	x3, tpidr_el1
	ldr	x3, [x3, #CI_CURLWP]
	adr	x4, .Lcopyin_onfault
	str	x4, [x3, #L_MD_ONFAULT]
	/*
	 * How much to copy?  Less than 32 bytes?  ...
	 */
	lsr	x4, x2, #5
	cbz	x4, .Lcopyin_last_suboword

	/*
	 * Subtract the amount of data we copied to get a quadword aligned
	 * from the length: length -= 16 - offset -> length += offset - 16;
	 */
	and	x5, x0, #15
	cbz	x5, .Lcopyin_loop

	add	x2, x2, x5
	sub	x2, x2, #16

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
	tbz	x0, #3, .Lcopyin_dword_aligned
	ldtr	x5, [x0]
	add	x0, x0, #8
	str	x5, [x1], #8
.Lcopyin_qword_aligned:
	lsr	x4, x2, #5
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
	tbz	x2, #5, .Lcopyin_last_subqword
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
	tbz	x2, #2, .Lcopyin_last_subdword
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
	str	xzr, [x3, #L_MD_ONFAULT]
	mov	x0, xzr
	ret

.Lcopyin_onfault:
	str	xzr, [x3, #L_MD_ONFAULT]
	mov	x0, #EFAULT
	ret
END(copyin)

/* LINTSTUB: int copyout(const void *kaddr, void *uaddr, size_t len); */

ENTRY(copyout)
	ret
END(copyout)

/* LINTSTUB: int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done); */

ENTRY(copystr)
	ret
END(copystr)

/* LINTSTUB: int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done); */

ENTRY(copyinstr)
	ret
END(copyinstr)

/* LINTSTUB: int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done); */

ENTRY(copyoutstr)
	ret
END(copyoutstr)

/* LINSTSTUB: int kcopy(const void *src, void *dst, size_t len); */

ENTRY(kcopy)
	ret
END(kcopy)

