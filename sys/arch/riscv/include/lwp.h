/* $NetBSD: lwp.h,v 1.1 2024/11/03 22:24:22 christos Exp $ */

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
#ifndef _RISCV_LWP_H_
#define _RISCV_LWP_H_

#include <sys/tls.h>

static __inline void *
__lwp_getprivate_fast(void)
{
	void *__tp;
	__asm("mv %0, tp" : "=r"(__tp));
	return __tp;
}

/*
 * On RISCV, since displacements are signed 12-bit values, the TCB Pointer
 * is biased by sizeof(tcb) so that first thread datum can be addressed by
 * -sizeof(tcb).
 */

#define	TLS_TP_OFFSET	0x0
#define	TLS_TCB_ALIGN	16
#define	TLS_DTV_OFFSET	0x800
__CTASSERT(TLS_TP_OFFSET + sizeof(struct tls_tcb) < 0x800);

static __inline void *
__lwp_gettcb_fast(void)
{
	void *__tcb;

	__asm __volatile(
		"addi %[__tcb], tp, %[__offset]"
	    :	[__tcb] "=r" (__tcb)
	    :	[__offset] "n" (-(TLS_TP_OFFSET + sizeof(struct tls_tcb))));

	return __tcb;
}

static __inline void
__lwp_settcb(void *__tcb)
{
	__asm __volatile(
		"addi tp, %[__tcb], %[__offset]"
	    :
	    :	[__tcb] "r" (__tcb),
		[__offset] "n" (TLS_TP_OFFSET + sizeof(struct tls_tcb)));
}


#endif /* !_RISCV_LWP_H_ */
