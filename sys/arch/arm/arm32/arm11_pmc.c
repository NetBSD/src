/*	$NetBSD: arm11_pmc.c,v 1.3.40.1 2014/08/20 00:02:45 tls Exp $	*/

/* Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * support for ARM 11 Performance Monitor Counters
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm11_pmc.c,v 1.3.40.1 2014/08/20 00:02:45 tls Exp $");
#include "opt_perfctrs.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>  
#include <sys/time.h>
#include <sys/timetc.h>
#include <dev/clock_subr.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#ifndef ARM11_PMC_CCNT_HZ
# define ARM11_PMC_CCNT_HZ	400000000	/* 400MHz */
#endif

void arm11_pmc_ccnt_init(void);

#define COUNTS_PER_USEC	(ARM11_PMC_CCNT_HZ / 1000000)

static uint32_t counts_per_wrap = ~0UL;		/* XXX off by 1 */

static inline uint32_t
arm11_pmc_ctrl_read(void)
{
	uint32_t val;

	__asm volatile ("mrc p15, 0, %0, c15, c12, 0" : "=r" (val));

	return val;
}

static inline void
arm11_pmc_ctrl_write(uint32_t val)
{
	__asm volatile ("mcr p15, 0, %0, c15, c12, 0" :: "r" (val));
}

static inline uint32_t
arm11_pmc_ccnt_read(void)
{
	uint32_t val;

	__asm volatile ("mrc p15, 0, %0, c15, c12, 1" : "=r" (val));

	return val;
}

__unused static inline void
arm11_pmc_ccnt_write(uint32_t val)
{
	__asm volatile ("mcr p15, 0, %0, c15, c12, 1;" :: "r" (val));
}

/*
 * enable the PMC CCNT for delay()
 */
void
arm11_pmc_ccnt_init(void)
{
	uint32_t val;

	val = ARM11_PMCCTL_E | ARM11_PMCCTL_P | ARM11_PMCCTL_C;

	arm11_pmc_ctrl_write(val);
}

/*
 * delay - for "at least" arg usec
 *
 *	NOTE: at 400MHz we are restricted to (uint32_t)~0 "counts"
 *	if this is a problem, accumulate counts in LL vars
 */
#define DELAY_ARG_LIMIT (((uint32_t)~0) / COUNTS_PER_USEC)	/* about 10 sec */
void
delay(u_int arg)
{
	uint32_t ctrl;
	uint32_t cur;
	uint32_t last;
	uint32_t delta = 0;
	uint32_t usecs = 0;

	if (arg > DELAY_ARG_LIMIT)
		panic("delay: arg %u overflow, limit is %d usec\n", arg, DELAY_ARG_LIMIT);

	last = arm11_pmc_ccnt_read();
	delta = usecs = 0;

	while (arg > usecs) {
		cur  = arm11_pmc_ccnt_read();
		ctrl = arm11_pmc_ctrl_read();
		if (ctrl & ARM11_PMCCTL_CCR) {
			/*
			 * reset CCR, do not reset other write-to-clear flags;
			 * keep the rest of the PMC Control Reg configuration
			 */
			ctrl &= ~(ARM11_PMCCTL_CR0|ARM11_PMCCTL_CR1);
			arm11_pmc_ctrl_write(ctrl);
			delta += (last + (counts_per_wrap - cur));
		} else {
			delta += (cur - last);
		}
		last = cur;
		if (delta >= COUNTS_PER_USEC) {
			usecs += delta / COUNTS_PER_USEC;
			delta %= COUNTS_PER_USEC;
		}
	}
}
