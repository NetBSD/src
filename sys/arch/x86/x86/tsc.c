/* $NetBSD: tsc.c,v 1.4 2006/06/13 17:30:05 dogcow Exp $ */


/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * re-implementation of TSC for MP systems merging cc_microtime and
 * TSC for timecounters by Frank Kardel
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/* basic calibration ideas are (kern_microtime.c): */
/******************************************************************************
 *                                                                            *
 * Copyright (c) David L. Mills 1993, 1994                                    *
 *                                                                            *
 * Permission to use, copy, modify, and distribute this software and its      *
 * documentation for any purpose and without fee is hereby granted, provided  *
 * that the above copyright notice appears in all copies and that both the    *
 * copyright notice and this permission notice appear in supporting           *
 * documentation, and that the name University of Delaware not be used in     *
 * advertising or publicity pertaining to distribution of the software        *
 * without specific, written prior permission.  The University of Delaware    *
 * makes no representations about the suitability this software for any       *
 * purpose.  It is provided "as is" without express or implied warranty.      *
 *                                                                            *
 ******************************************************************************/

/* reminiscents from older version of this file are: */
/*-
 * Copyright (c) 1998-2003 Poul-Henning Kamp
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

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/i386/i386/tsc.c,v 1.204 2003/10/21 18:28:34 silby Exp $"); */
__KERNEL_RCSID(0, "$NetBSD: tsc.c,v 1.4 2006/06/13 17:30:05 dogcow Exp $");

#include "opt_multiprocessor.h"
#include "opt_enhanced_speedstep.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/power.h>
#include <sys/reboot.h>	/* XXX for bootverbose */
#include <machine/cpu.h>
#include <machine/cpu_counter.h>
#include <x86/x86/tsc.h>
#include <machine/specialreg.h>

uint64_t	tsc_freq;
u_int		tsc_present;
int		tsc_is_broken = 0;

static int64_t tsc_cal_val;  /* last calibrate time stamp */

static timecounter_get_t tsc_get_timecount;
static timecounter_pps_t tsc_calibrate;

void tsc_calibrate_cpu(struct cpu_info *);

static struct timecounter tsc_timecounter = {
	tsc_get_timecount,	/* get_timecount */
	tsc_calibrate,		/* once per second - used to calibrate cpu TSC */
 	~0u,			/* counter_mask */
	0,			/* frequency */
	 "TSC",			/* name */
#if defined(ENHANCED_SPEEDSTEP)
	-100,			/* don't pick TSC automatically */
				/* if frequency changes might affect TSC */
#else
	800,			/* quality (adjusted in code) */
#endif
};

void
init_TSC(void)
{
	u_int64_t tscval[2];

	if (cpu_feature & CPUID_TSC)
		tsc_present = 1;
	else
		tsc_present = 0;

	if (!tsc_present) 
		return;

	if (bootverbose)
	        printf("Calibrating TSC clock ... ");

	do {
		tscval[0] = rdtsc();
		DELAY(100000);
		tscval[1] = rdtsc();
	} while (tscval[1] < tscval[0]);

	tsc_freq = 10 * (tscval[1] - tscval[0]);
	if (bootverbose)
		printf("TSC clock: %" PRId64 " Hz\n", tsc_freq);
}

void
init_TSC_tc(void)
{
	if (tsc_present && tsc_freq != 0 && !tsc_is_broken) {
		tsc_timecounter.tc_frequency = tsc_freq;
		tc_init(&tsc_timecounter);
	}
}

/* XXX make tsc_timecounter.tc_frequency settable by sysctl() */

/*
 * pick up tick count scaled to reference tick count
 */
static u_int
tsc_get_timecount(struct timecounter *tc)
{
	struct cpu_info *ci = curcpu();
	int64_t rcc, cc;
	u_int gen;

	if (ci->ci_cc.cc_denom == 0) {
		/*
		 * This is our first time here on this CPU.  Just
		 * start with reasonable initial values.
		 */
	        ci->ci_cc.cc_cc    = cpu_counter32();
		ci->ci_cc.cc_val   = 0;
		if (ci->ci_cc.cc_gen == 0)
			ci->ci_cc.cc_gen++;

		ci->ci_cc.cc_denom = cpu_frequency(ci);
		if (ci->ci_cc.cc_denom == 0)
			ci->ci_cc.cc_denom = tsc_freq;
		ci->ci_cc.cc_delta = ci->ci_cc.cc_denom;
	}

	/* read counter and re-read when the re-calibration
	   strikes inbetween */
	do {
		/* pick up current generation number */
		gen = ci->ci_cc.cc_gen;

		/* determine local delta ticks */
		cc = cpu_counter32() - ci->ci_cc.cc_cc;
		if (cc < 0)
			cc += 0x100000000LL;

		/* scale to primary */
		rcc = (cc * ci->ci_cc.cc_delta) / ci->ci_cc.cc_denom
			+ ci->ci_cc.cc_val;
	} while (gen == 0 || gen != ci->ci_cc.cc_gen);

	return rcc;
}

/*
 * called once per second via the pps callback
 * for the calibration of the TSC counters.
 * it is called only for the PRIMARY cpu. all
 * other cpus are called via a broadcast IPI
 */
static void
tsc_calibrate(struct timecounter *tc)
{
	struct cpu_info *ci = curcpu();
	
	/* pick up reference ticks */
	tsc_cal_val = cpu_counter32();

#if defined(MULTIPROCESSOR)
	x86_broadcast_ipi(X86_IPI_MICROSET);
#endif

	tsc_calibrate_cpu(ci);
}

/*
 * This routine is called about once per second directly by the master
 * processor and via an interprocessor interrupt for other processors.
 * It determines the CC frequency of each processor relative to the
 * master clock and the time this determination is made.  These values
 * are used by tsc_get_timecount() to interpolate the ticks between
 * timer interrupts.  Note that we assume the kernel variables have
 * been zeroed early in life.
 */
void
tsc_calibrate_cpu(struct cpu_info *ci)
{
	u_int   gen;
	int64_t val;
	int64_t delta, denom;
	int s;
#ifdef TIMECOUNTER_DEBUG
	int64_t factor, old_factor;
#endif
	val = tsc_cal_val;

	s = splhigh();
	/* create next generation number */
	gen = ci->ci_cc.cc_gen;
	gen++;
	if (gen == 0)
		gen++;
	/* update in progress */
	ci->ci_cc.cc_gen = 0;

	denom = ci->ci_cc.cc_cc;
	ci->ci_cc.cc_cc = cpu_counter32();

	if (ci->ci_cc.cc_denom == 0) {
		/*
		 * This is our first time here on this CPU.  Just
		 * start with reasonable initial values.
		 */
		ci->ci_cc.cc_val = val;
		ci->ci_cc.cc_denom = cpu_frequency(ci);
		if (ci->ci_cc.cc_denom == 0)
			ci->ci_cc.cc_denom = tsc_freq;
		ci->ci_cc.cc_delta = ci->ci_cc.cc_denom;
		ci->ci_cc.cc_gen = gen;
		splx(s);
		return;
	}

#ifdef TIMECOUNTER_DEBUG
	old_factor = (ci->ci_cc.cc_delta * 1000 ) / ci->ci_cc.cc_denom;
#endif

	/* local ticks per period */
	denom = ci->ci_cc.cc_cc - denom;
	if (denom < 0)
		denom += 0x100000000LL;

	ci->ci_cc.cc_denom = denom;

	/* reference ticks per period */
	delta = val - ci->ci_cc.cc_val;
	if (delta < 0)
		delta += 0x100000000LL;

	ci->ci_cc.cc_val = val;
	ci->ci_cc.cc_delta = delta;
	
	/* publish new generation number */
	ci->ci_cc.cc_gen = gen;
	splx(s);

#ifdef TIMECOUNTER_DEBUG
	factor = (delta * 1000) / denom - old_factor;
	if (factor < 0)
		factor = -factor;

	if (factor > old_factor / 10)
		printf("tsc_calibrate_cpu[%lu]: 10%% exceeded - delta %"
		       PRId64 ", denom %" PRId64 ", factor %" PRId64
		       ", old factor %" PRId64"\n", ci->ci_cpuid,
		       delta, denom, (delta * 1000) / denom, old_factor);
#if 0
	printf("tsc_calibrate_cpu[%lu]: delta %" PRId64
	       ", denom %" PRId64 ", factor %" PRId64 "\n", ci->ci_cpuid, delta, denom, (delta * 1000) / denom);
#endif
#endif /* TIMECOUNTER_DEBUG */
}
