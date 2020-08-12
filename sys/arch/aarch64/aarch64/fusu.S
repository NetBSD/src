/*	$NetBSD: fusu.S,v 1.10 2020/08/12 13:19:35 skrll Exp $	*/

/*-
 * Copyright (c) 2014, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, and by Jason R. Thorpe.
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

RCSID("$NetBSD: fusu.S,v 1.10 2020/08/12 13:19:35 skrll Exp $");

#ifdef ARMV81_PAN
#define PAN_ENABLE	\
	adrl	x9, _C_LABEL(aarch64_pan_enabled)	; \
	ldr	w9, [x9]				; \
	cbz	w9, 666f				; \
	msr	pan, #1					; \
666:
#define PAN_DISABLE	\
	adrl	x9, _C_LABEL(aarch64_pan_enabled)	; \
	ldr	w9, [x9]				; \
	cbz	w9, 666f				; \
	msr	pan, #0					; \
666:
#else
#define PAN_ENABLE	/* nothing */
#define PAN_DISABLE	/* nothing */
#endif

	ARMV8_DEFINE_OPTIONS

	.macro enter_cpu_onfault
	stp	fp, lr, [sp, #-16]!	/* save fp, lr */
	mov	fp, sp			/* link frame pointer */

	stp	x19, x20, [sp, #-16]!	/* save x19, x20 */
	mov	x19, x0			/* x19 = arg0 */
	mov	x20, x1			/* x20 = arg1 */

	/* if (cpu_set_onfault(fb) != 0) return error */
	sub	sp, sp, #FB_T_SIZE	/* allocate struct faultbuf */
	mov	x0, sp			/* x0 = faultbuf */
	bl	cpu_set_onfault		/* x0 = cpu_set_onfault() */
	cbnz	x0, 9f			/* return if error */

	PAN_DISABLE			/* disable PAN */
	.endm

	.macro exit_cpu_onfault
	/* curlwp->l_md.md_onfault = NULL */
	mrs	x1, tpidr_el1			/* x1 = curlwp */
	str	xzr, [x1, #L_MD_ONFAULT]	/* lwp->l_md_onfault = NULL */
9:
	PAN_ENABLE				/* enable PAN */
	add	sp, sp, #FB_T_SIZE		/* pop stack */
	ldp	x19, x20, [sp], #16		/* restore x19, x20 */
	ldp	fp, lr, [sp], #16		/* restore fp, lr */
	.endm

/* LINTSTUB: int _ufetch_8(const uint8_t *uaddr, uint8_t *valp); */
ENTRY(_ufetch_8)
	enter_cpu_onfault

	ldtrb	w8, [x19]
	strb	w8, [x20]

	exit_cpu_onfault
	ret
END(_ufetch_8)

/* LINTSTUB: int _ufetch_16(const uint16_t *uaddr, uint16_t *valp); */
ENTRY(_ufetch_16)
	enter_cpu_onfault

	ldtrh	w8, [x19]
	strh	w8, [x20]

	exit_cpu_onfault
	ret
END(_ufetch_16)

/* LINTSTUB: int _ufetch_32(const uint32_t *uaddr, uint32_t *valp); */
ENTRY(_ufetch_32)
	enter_cpu_onfault

	ldtr	w8, [x19]
	str	w8, [x20]

	exit_cpu_onfault
	ret
END(_ufetch_32)

/* LINTSTUB: int _ufetch_64(const uint64_t *uaddr, uint64_t *valp); */
ENTRY(_ufetch_64)
	enter_cpu_onfault

	ldtr	x8, [x19]
	str	x8, [x20]

	exit_cpu_onfault
	ret
END(_ufetch_64)

/* LINTSTUB: int _ustore_8(uint8_t *uaddr, uint8_t val); */
ENTRY(_ustore_8)
	enter_cpu_onfault

	sttrb	w20, [x19]

	exit_cpu_onfault
	ret
END(_ustore_8)

/* LINTSTUB: int _ustore_16(uint16_t *uaddr, uint16_t val); */
ENTRY(_ustore_16)
	enter_cpu_onfault

	sttrh	w20, [x19]

	exit_cpu_onfault
	ret
END(_ustore_16)

/* LINTSTUB: int _ustore_32(uint32_t *uaddr, uint32_t val); */
ENTRY(_ustore_32)
	enter_cpu_onfault

	sttr	w20, [x19]

	exit_cpu_onfault
	ret
END(_ustore_32)

/* LINTSTUB: int _ustore_64(uint64_t *uaddr, uint64_t val); */
ENTRY(_ustore_64)
	enter_cpu_onfault

	sttr	x20, [x19]

	exit_cpu_onfault
	ret
END(_ustore_64)
