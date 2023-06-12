/*	$NetBSD: clock_machdep.c,v 1.5 2023/06/12 19:04:14 skrll Exp $	*/

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

#include <sys/cdefs.h>

__RCSID("$NetBSD: clock_machdep.c,v 1.5 2023/06/12 19:04:14 skrll Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <machine/machdep.h>
#include <machine/sbi.h>
#include <machine/sysreg.h>

static void (*_riscv_timer_init)(void) = riscv_timer_init;

static uint32_t timer_frequency;
static uint32_t	timer_ticks_per_hz;

static u_int
timer_get_timecount(struct timecounter *tc)
{
	return csr_time_read();
}

static struct timecounter tc =  {
	.tc_get_timecount = timer_get_timecount,
	.tc_counter_mask = ~0u,
	.tc_name = "riscv",
	.tc_quality = 100,
};


void
riscv_timer_frequency_set(uint32_t freq)
{
	timer_frequency = freq;
	timer_ticks_per_hz = freq / hz;
}

uint32_t
riscv_timer_frequency_get(void)
{
	return timer_frequency;
}

void
riscv_timer_register(void (*timerfn)(void))
{
       if (_riscv_timer_init != NULL) {
#ifdef DIAGNOSTIC
               aprint_verbose("%s: timer already registered\n", __func__);
#endif
       }
	_riscv_timer_init = timerfn;
}

void
riscv_timer_init(void)
{
	struct cpu_info * const ci = curcpu();

	ci->ci_lastintr = csr_time_read();
	uint64_t next = ci->ci_lastintr + timer_ticks_per_hz;
	ci->ci_lastintr_scheduled = next;

	sbi_set_timer(next);		/* schedule next timer interrupt */
	csr_sie_set(SIE_STIE);		/* enable supervisor timer intr */

	if (cpu_index(ci) == 0) {
		tc.tc_frequency = timer_frequency;
		tc_init(&tc);
	}
}


int
riscv_timer_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	struct clockframe * const cf = arg;

	csr_sip_clear(SIP_STIP);	/* clean pending interrupt status */

	const uint64_t now = csr_time_read();

	ci->ci_lastintr = now;
	ci->ci_ev_timer.ev_count++;

	ci->ci_lastintr_scheduled += timer_ticks_per_hz;
	sbi_set_timer(ci->ci_lastintr_scheduled);

	hardclock(cf);

	return 1;
}


void
cpu_initclocks(void)
{
	if (_riscv_timer_init == NULL)
		panic("cpu_initclocks: no timer registered");
	_riscv_timer_init();
}


void
setstatclockrate(int newhz)
{
}
