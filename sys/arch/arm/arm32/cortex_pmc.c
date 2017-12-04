/*	$NetBSD: cortex_pmc.c,v 1.5 2017/12/04 08:24:35 skrll Exp $	*/

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
 * support for ARM cortex Performance Monitor Counters
 * based on arm11_pmc.c
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: cortex_pmc.c,v 1.5 2017/12/04 08:24:35 skrll Exp $"); */
#include "opt_perfctrs.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <uvm/uvm_extern.h>

#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/arm32/machdep.h>

#ifndef CORTEX_PMC_CCNT_HZ
# define CORTEX_PMC_CCNT_HZ	400000000	/* 400MHz */
#endif

#define COUNTS_PER_USEC	(curcpu()->ci_data.cpu_cc_freq / (1000*1000))

static const uint32_t counts_per_wrap = ~0UL - 1;

/*
 * enable the PMC CCNT for delay()
 */
void
cortex_pmc_ccnt_init(void)
{
	if (curcpu()->ci_data.cpu_cc_freq == 0) {
		curcpu()->ci_data.cpu_cc_freq = CORTEX_PMC_CCNT_HZ;
	}
}

/*
 * delay - for "at least" arg usec
 *
 *	NOTE: at 400MHz we are restricted to (uint32_t)~0 "counts"
 *	if this is a problem, accumulate counts in LL vars
 */
void
delay(u_int arg)
{
	uint32_t ctrl;
	uint32_t cur;
	uint32_t last;
	uint32_t delta = 0;
	uint32_t usecs = 0;
	const uint32_t counts_per_usec = COUNTS_PER_USEC;
	const uint32_t delay_arg_limit = ~0UL / counts_per_usec; /* about 10 sec */

	if (arg > delay_arg_limit)
		panic("%s: arg %u overflow, limit is %u usec\n",
		    __func__, arg, delay_arg_limit);

	last = armreg_pmccntr_read();
	delta = usecs = 0;
	while (arg > usecs) {
		cur = armreg_pmccntr_read();

		/*
		 * overflow flag is moved to a separate register
		 * and is not read from PMC Control Register
		 */
		ctrl = armreg_pmovsr_read();
		if (ctrl & CORTEX_CNTOFL_C) {
			/*
			 * Reset overflow flag for cycle counter in overflow
			 * register
			 */
			armreg_pmovsr_write(CORTEX_CNTOFL_C);
			delta += (cur + (counts_per_wrap - last));
		} else {
			delta += (cur - last);
		}
		last = cur;
		if (delta >= counts_per_usec) {
			usecs += delta / counts_per_usec;
			delta %= counts_per_usec;
		}
	}
}
