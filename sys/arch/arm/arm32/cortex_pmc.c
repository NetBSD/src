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
 * support for ARM cortexa8 Performance Monitor Counters
 * based on arm11_pmc.c
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: cortex_pmc.c,v 1.1 2010/06/19 19:44:57 matt Exp $"); */
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

#ifndef CORTEX_PMC_CCNT_HZ
# define CORTEX_PMC_CCNT_HZ	400000000	/* 400MHz */
#endif

void cortexa8_pmc_ccnt_init(void);

#define COUNTS_PER_USEC	(CORTEX_PMC_CCNT_HZ / 1000000)

static uint32_t counts_per_wrap = ~0UL;		/* XXX off by 1 */

#define PMNC "c9, c12, 0"
#define CCNT "c9, c13, 0"

static inline uint32_t
cortexa8_pmc_ctrl_read(void)
{
	uint32_t val;

	__asm volatile ("mrc p15, 0, %0, " PMNC : "=r" (val));

	return val;
}

static inline void
cortexa8_pmc_ctrl_write(uint32_t val)
{
	__asm volatile ("mcr p15, 0, %0, " PMNC :: "r" (val));
}

static inline uint32_t
cortexa8_pmc_ccnt_read(void)
{
	uint32_t val;

	__asm volatile ("mrc p15, 0, %0, " CCNT : "=r" (val));

	return val;
}

static inline void
cortexa8_pmc_ccnt_write(uint32_t val)
{
  __asm volatile ("mcr p15, 0, %0, c9, c12, 2"  :: "r" (CORTEX_CNTENC_C));
  __asm volatile ("mcr p15, 0, %0, " CCNT :: "r" (val));
  __asm volatile ("mcr p15, 0, %0, c9, c12, 1" :: "r" (CORTEX_CNTENS_C));
}

/*
 * enable the PMC CCNT for delay()
 */
void
cortexa8_pmc_ccnt_init(void)
{
  uint32_t val;
  
  val = ARM11_PMCCTL_E | ARM11_PMCCTL_P | ARM11_PMCCTL_C;
	
  cortexa8_pmc_ctrl_write(val);
  __asm volatile ("mcr p15, 0, %0, c9, c12, 1" :: "r" (CORTEX_CNTENS_C));
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

	last = cortexa8_pmc_ccnt_read();
	delta = usecs = 0;
	while (arg > usecs) {
		cur  = cortexa8_pmc_ccnt_read();
		
		/* overflow flag is moved to a separate register
		   and is not read from PMC Control Register */
		__asm volatile ("mrc p15, 0, %0, c9, c12, 3" : "=r" (ctrl));
		if(ctrl & CORTEX_CNTOFL_C){
		  /* Reset overflow flag for cycle counter in overflow register */
		  __asm volatile ("mcr p15, 0, %0, c9, c12, 3" :: "r" (CORTEX_CNTOFL_C));
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
