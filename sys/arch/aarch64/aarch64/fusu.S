/*	$NetBSD: fusu.S,v 1.4 2018/07/17 10:01:59 ryo Exp $	*/

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

RCSID("$NetBSD: fusu.S,v 1.4 2018/07/17 10:01:59 ryo Exp $");

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
	mov	x0, sp
	bl	cpu_set_onfault
	mvn	x8, xzr			/* temporary return value = -1 */
	cbnz	w0, 9f			/* return if error */
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

/* LINTSTUB: int fubyte(const void *base); */
ENTRY(fubyte)
	enter_cpu_onfault

	ldtrb	w8, [x19]

	exit_cpu_onfault
	ret
END(fubyte)

/* LINTSTUB: int fusword(const void *base); */
ENTRY(fusword)
	enter_cpu_onfault

	ldtrh	w8, [x19]

	exit_cpu_onfault
	ret
END(fusword)

/* LINTSTUB: int fuswintr(const void *base); */
ENTRY(fuswintr)
	enter_cpu_onfault

	ldtrh	w8, [x19]

	exit_cpu_onfault
	ret
END(fuswintr)

/* LINTSTUB: int fuword32(const void *base); */
ENTRY(fuword32)
	enter_cpu_onfault

	ldtr	w8, [x19]

	exit_cpu_onfault
	ret
END(fuword32)

/* LINTSTUB: long fuword(const void *base); */
ENTRY(fuword)
	enter_cpu_onfault

	ldtr	x8, [x19]

	exit_cpu_onfault
	ret
END(fuword)

/* LINTSTUB: int subyte(void *base, int c); */
ENTRY(subyte)
	enter_cpu_onfault

	sttrb	w20, [x19]
	mov	x8, #0

	exit_cpu_onfault
	ret
END(subyte)

/* LINTSTUB: int suword(void *base, short c); */
ENTRY(susword)
	enter_cpu_onfault

	sttrh	w20, [x19]
	mov	x8, #0

	exit_cpu_onfault
	ret
END(susword)

/* LINTSTUB: int suswintr(void *base, short c); */
ENTRY(suswintr)
	enter_cpu_onfault

	sttrh	w20, [x19]
	mov	x8, #0

	exit_cpu_onfault
	ret
END(suswintr)

/* LINTSTUB: int suword32(void *base, int c); */
ENTRY(suword32)
	enter_cpu_onfault

	sttr	w20, [x19]
	mov	x8, #0

	exit_cpu_onfault
	ret
END(suword32)

/* LINTSTUB: int suword(void *base, long c); */
ENTRY(suword)
	enter_cpu_onfault

	sttr	x20, [x19]
	mov	x8, #0

	exit_cpu_onfault
	ret
END(suword)

