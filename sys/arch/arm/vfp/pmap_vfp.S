/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

#include "opt_cputypes.h"

#include <machine/asm.h>
#include "assym.h"

RCSID("$NetBSD: pmap_vfp.S,v 1.5 2012/12/26 18:35:47 matt Exp $")

/*
 * This zeroes a page 64-bytes at a time.  64 was chosen over 32 since
 * 64 is the cache line size of the Cortex-A8.
 */
/* LINTSTUB: void bzero_page_vfp(vaddr_t); */
ENTRY(bzero_page_vfp)
#if 0
	str	lr, [sp, #-8]!
	bl	_C_LABEL(vfp_kernel_acquire)
#else
	mrc	p10, 7, r3, c8, c0, 0
	tst	r3, #VFP_FPEXC_EN
	orreq	r2, r3, #VFP_FPEXC_EN
	mcreq	p10, 7, r2, c8, c0, 0
	vpush	{d0-d7}
#endif
#if (CPU_CORTEX == 0)
	mov	ip, #0
	vmov	s0, ip
	vmov	s1, ip
	vmov.f64 d1, d0
	vmov.f64 d2, d0
	vmov.f64 d3, d0
	vmov.f64 d4, d0
	vmov.f64 d5, d0
	vmov.f64 d6, d0
	vmov.f64 d7, d0
#else
	veor	q0, q0, q0
	veor	q1, q1, q1
	veor	q2, q2, q2
	veor	q3, q3, q3
#endif
	add	r2, r0, #PAGE_SIZE
1:	vstmia	r0!, {d0-d7}
	vstmia	r0!, {d0-d7}
	vstmia	r0!, {d0-d7}
	vstmia	r0!, {d0-d7}
	cmp	r0, r2
	blt	1b
#if 0
	ldr	lr, [sp], #8		/* fetch LR */
	b	_C_LABEL(vfp_kernel_release)	/* tailcall the vfp release */
#else
	vpop	{d0-d7}
	mcr	p10, 7, r3, c8, c0, 0
	RET
#endif
END(bzero_page_vfp)

/*
 * This copies a page 64-bytes at a time.  64 was chosen over 32 since
 * 64 is the cache line size of the Cortex-A8.
 */
/* LINTSTUB: void bcopy_page_vfp(vaddr_t, vaddr_t); */
ENTRY(bcopy_page_vfp)
#ifdef _ARM_ARCH_DWORD_OK
	pld	[r0]			@ preload the first 128 bytes
	pld	[r0, #32]
	pld	[r0, #64]
	pld	[r0, #96]
#endif
#if 0
	str	lr, [sp, #-8]!
	bl	_C_LABEL(vfp_kernel_acquire)
#else
	mrc	p10, 7, r3, c8, c0, 0
	tst	r3, #VFP_FPEXC_EN
	orreq	r2, r3, #VFP_FPEXC_EN
	mcreq	p10, 7, r2, c8, c0, 0
	vpush	{d0-d7}
	add	r2, r0, #PAGE_SIZE-128
#endif
1:	
#ifdef _ARM_ARCH_DWORD_OK
	pld	[r0, #128]		@ preload the next 128
	pld	[r0, #160]
	pld	[r0, #192]
	pld	[r0, #224]
#endif
2:	vldmia	r0!, {d0-d7}		@ read   0-63
	vstmia	r1!, {d0-d7}		@ write  0-63
	vldmia	r0!, {d0-d7}		@ read  64-127
	vstmia	r1!, {d0-d7}		@ write 64-127
	cmp	r0, r2
	blt	1b
	beq	2b
#if 0
	ldr	lr, [sp], #8		/* fetch LR */
	b	_C_LABEL(vfp_kernel_release)	/* tailcall the vfp release */
#else
	vpop	{d0-d7}
	mcr	p10, 7, r3, c8, c0, 0
	RET
#endif
END(bcopy_page_vfp)
