/*	$NetBSD: lwp.h,v 1.1 2024/11/03 22:24:22 christos Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _POWERPC_LWP_H_
#define _POWERPC_LWP_H_

#include <sys/tls.h>

/*
 * On PowerPC, since displacements are signed 16-bit values, the TCB Pointer
 * is biased by 0x7000 + sizeof(tcb) so that first thread datum can be 
 * addressed by -28672 thereby leaving 60KB available for use as thread data.
 */
#define	TLS_TP_OFFSET	0x7000
#define	TLS_DTV_OFFSET	0x8000
__CTASSERT(TLS_TP_OFFSET + sizeof(struct tls_tcb) < 0x8000);

__BEGIN_DECLS

static __inline void *
__lwp_gettcb_fast(void)
{
	void *__tcb;

	__asm __volatile(
		"addi %[__tcb],%%r2,%[__offset]"
	    :	[__tcb] "=r" (__tcb)
	    :	[__offset] "n" (-(TLS_TP_OFFSET + sizeof(struct tls_tcb))));

	return __tcb;
}

static __inline void
__lwp_settcb(void *__tcb)
{
	__tcb = (uint8_t *)__tcb + TLS_TP_OFFSET + sizeof(struct tls_tcb);

	__asm __volatile(
		"mr %%r2,%[__tcb]"
	    :
	    :	[__tcb] "r" (__tcb));

	_lwp_setprivate(__tcb);
}
__END_DECLS

#endif	/* !_POWERPC_LWP_H_ */
