/*	$NetBSD: cpu_counter.h,v 1.7.4.1 2011/06/06 09:06:27 jruoho Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POWERPC_CPU_COUNTER_H_
#define _POWERPC_CPU_COUNTER_H_

/*
 * Machine-specific support for CPU counter.
 */

#ifdef _KERNEL

#include <powerpc/spr.h>
#ifdef PPC_OEA601
#include <powerpc/oea/spr.h>
#endif

#define cpu_hascounter()	(1)
#define cpu_counter()		cpu_counter32()

static __inline uint32_t
cpu_counter32(void)
{
	uint32_t rv, rtcu, scratch;

	__asm volatile (
#ifdef PPC_OEA601
	    "mfpvr	%0		\n"
	    "srwi	%0,%0,16	\n"
	    "cmpwi	%0,%3		\n"
	    "bne	1f		\n"	/* branch if not 601 */
	    "lis	%0,0x77		\n"
	    "ori	%0,%0,0x3594	\n"	/* 7.8125e6 */
	    "mfspr	%2,%4		\n"
	    "mullw	%2,%2,%0	\n"
	    "mfspr	%0,%5		\n"
	    "srwi	%0,%0,7		\n"
	    "add	%1,%2,%0	\n"
	    "b		2f		\n"
	    "1:				\n"
#elif defined(PPC_IBM403)
	    "mftblo	%1		\n"
#elif defined(PPC_BOOKE)
	    "mfspr	%1,%3		\n"
#else
	    "mftb	%1		\n"
#endif
	    "2:				\n"
		: "=r"(scratch), "=r"(rv), "=r"(rtcu)
#ifdef PPC_OEA601
		: "n"(MPC601), "n"(SPR_RTCU_R), "n"(SPR_RTCL_R)
#elif defined(PPC_BOOKE)
		: "n"(SPR_TBL)
#endif
	    );

	return rv;
}

extern uint32_t ticks_per_sec;

static __inline uint64_t
cpu_frequency(struct cpu_info *ci)
{
	/* XXX this probably only works on 603 and newer */
	return ticks_per_sec;
}

#endif /* _KERNEL */

#endif /* _POWERPC_CPU_COUNTER_H_ */
