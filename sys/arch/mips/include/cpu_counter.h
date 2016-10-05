/*	$NetBSD: cpu_counter.h,v 1.4.32.1 2016/10/05 20:55:31 skrll Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _MIPS_CPU_COUNTER_H_
#define _MIPS_CPU_COUNTER_H_

/*
 * Machine-specific support for CPU counter.
 */

#ifdef _KERNEL
#if __mips < 3
#include <mips/locore.h>
#endif

static __inline int
cpu_hascounter(void)
{
	/*
	 * MIPS III and MIPS IV CPU's have a cycle counter
	 * running at half the internal pipeline rate.
	 */
#if __mips >= 3
	return 1;
#elif !defined(__mips_o32) || defined(MIPS3_PLUS)
	return (MIPS_HAS_CLOCK);
#else
	return 0;
#endif
}

#if __mips >= 3 || !defined(__mips_o32) || defined(MIPS3_PLUS)
#define cpu_counter()		cpu_counter32()

uint32_t cpu_counter32(void);	/* weak alias of mips3_cp0_count_read */
#else
#define	cpu_counter()		(0)
#define cpu_counter32()		(0)
#endif

static __inline uint64_t
cpu_frequency(struct cpu_info *ci)
{
	return ci->ci_cctr_freq;
}

#endif /* _KERNEL */
#endif /* !_MIPS_CPU_COUNTER_H_ */
