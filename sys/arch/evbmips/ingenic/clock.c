/*	$NetBSD: clock.c,v 1.1.2.4 2016/10/05 20:55:27 skrll Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.1.2.4 2016/10/05 20:55:27 skrll Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <mips/ingenic/ingenic_regs.h>

#include "opt_ingenic.h"

extern void ingenic_puts(const char *);

void ingenic_clockintr(struct clockframe *);

static u_int
ingenic_count_read(struct timecounter *tc)
{
	return readreg(JZ_OST_CNT_LO);
}

void
cpu_initclocks(void)
{
	struct cpu_info * const ci = curcpu();
	uint32_t cnt;

	static struct timecounter tc =  {
		ingenic_count_read,		/* get_timecount */
		0,				/* no poll_pps */
		~0u,				/* counter_mask */
		12000000,			/* frequency */
		"Ingenic OS timer",		/* name */
		100,				/* quality */
	};

	curcpu()->ci_cctr_freq = tc.tc_frequency;

	tc_init(&tc);

	printf("starting timer interrupt...\n");
	/* start the timer interrupt */
	cnt = readreg(JZ_OST_CNT_LO);
	ci->ci_next_cp0_clk_intr = cnt + ci->ci_cycles_per_hz;
	writereg(JZ_TC_TFCR, TFR_OSTFLAG);
	writereg(JZ_OST_DATA, ci->ci_next_cp0_clk_intr);
	/*
	 * XXX
	 * We can use OST or one of the regular timers to generate the 100hz
	 * interrupt. OST interrupts need to be rescheduled every time and by
	 * only one core, the regular timer can be programmed to fire every
	 * 10ms without rescheduling and we'd still use the OST as time base.
	 * OST is supposed to fire on INT2 although I haven't been able to get
	 * that to work yet ( all I get is INT0 which is for hardware interrupts
	 * in general )
	 * So if we can get OST to fire on INT2 we can just block INT0 on core1
	 * and have a timer interrupt on both cores, if not the regular timer
	 * would be more convenient but we'd have to shoot an IPI to core1 on
	 * every tick.
	 * For now, use OST and hope we'll figure out how to make it fire on
	 * INT2.
	 */
#ifdef USE_OST
	writereg(JZ_TC_TMCR, TFR_OSTFLAG);
#else
	writereg(JZ_TC_TECR, TESR_TCST5);	/* disable timer 5 */
	writereg(JZ_TC_TCNT(5), 0);
	writereg(JZ_TC_TDFR(5), 30000);	/* 10ms at 48MHz / 16 */
	writereg(JZ_TC_TDHR(5), 60000);	/* not reached */
	writereg(JZ_TC_TCSR(5), TCSR_EXT_EN| TCSR_DIV_16);
	writereg(JZ_TC_TMCR, TFR_FFLAG5);
	writereg(JZ_TC_TFCR, TFR_FFLAG5);
	writereg(JZ_TC_TESR, TESR_TCST5);	/* enable timer 5 */
#endif

#ifdef INGENIC_CLOCK_DEBUG
	printf("INTC %08x %08x\n", readreg(JZ_ICSR0), readreg(JZ_ICSR1));
	printf("ICMR0 %08x\n", readreg(JZ_ICMR0));
#endif
	writereg(JZ_ICMCR0, 0x0c000000); /* TCU2, OST */
	spl0();
#ifdef INGENIC_CLOCK_DEBUG
	printf("TFR: %08x\n", readreg(JZ_TC_TFR));
	printf("TMR: %08x\n", readreg(JZ_TC_TMR));
	printf("cnt5: %08x\n", readreg(JZ_TC_TCNT(5)));
	printf("CR: %08x\n", MFC0(MIPS_COP_0_CAUSE, 0));
	printf("SR: %08x\n", MFC0(MIPS_COP_0_STATUS, 0));
	delay(100000);
	printf("TFR: %08x\n", readreg(JZ_TC_TFR));
	printf("TMR: %08x\n", readreg(JZ_TC_TMR));
	printf("cnt5: %08x\n", readreg(JZ_TC_TCNT(5)));
	printf("CR: %08x\n", MFC0(MIPS_COP_0_CAUSE, 0));
	printf("SR: %08x\n", MFC0(MIPS_COP_0_STATUS, 0));
	printf("TFR: %08x\n", readreg(JZ_TC_TFR));
	printf("TMR: %08x\n", readreg(JZ_TC_TMR));
	printf("cnt5: %08x\n", readreg(JZ_TC_TCNT(5)));
	printf("CR: %08x\n", MFC0(MIPS_COP_0_CAUSE, 0));
	printf("SR: %08x\n", MFC0(MIPS_COP_0_STATUS, 0));

	printf("INTC %08x %08x\n", readreg(JZ_ICSR0), readreg(JZ_ICSR1));
	delay(3000000);
	printf("%s %d\n", __func__, MFC0(12, 3));
	printf("%s %08x\n", __func__, MFC0(12, 4));
#endif
}

/* shamelessly stolen from mips3_clock.c */
void
delay(int n)
{
	u_long divisor_delay;
	uint32_t cur, last, delta, usecs;

	last = readreg(JZ_OST_CNT_LO);
	delta = usecs = 0;

	divisor_delay = curcpu()->ci_divisor_delay;
	if (divisor_delay == 0) {
		/*
		 * Frequency values in curcpu() are not initialized.
		 * Assume faster frequency since longer delays are harmless.
		 * Note CPU_MIPS_DOUBLE_COUNT is ignored here.
		 */
#define FAST_FREQ	(300 * 1000 * 1000)	/* fast enough? */
		divisor_delay = FAST_FREQ / (1000 * 1000);
	}
	while (n > usecs) {
		cur = readreg(JZ_OST_CNT_LO);

		/*
		 * We setup the OS timer to always counts upto UINT32_MAX,
		 * so no need to check wrapped around case.
		 */
		delta += (cur - last);

		last = cur;

		while (delta >= divisor_delay) {
			/*
			 * delta is not so larger than divisor_delay here,
			 * and using DIV/DIVU ops could be much slower.
			 * (though longer delay may be harmless)
			 */
			usecs++;
			delta -= divisor_delay;
		}
	}
}

void
setstatclockrate(int r)
{
	/* we could just use another timer channel here */
}

#ifdef INGENIC_CLOCK_DEBUG
int cnt = 99;
#endif

void
ingenic_clockintr(struct clockframe *cf)
{
	int s = splsched();
	struct cpu_info * const ci = curcpu();
#ifdef USE_OST
	uint32_t new_cnt;
#endif

	/* clear flags */
	writereg(JZ_TC_TFCR, TFR_OSTFLAG);

	ci->ci_next_cp0_clk_intr += (uint32_t)(ci->ci_cycles_per_hz & 0xffffffff);
#ifdef USE_OST
	writereg(JZ_OST_DATA, ci->ci_next_cp0_clk_intr);

	/* Check for lost clock interrupts */
	new_cnt = readreg(JZ_OST_CNT_LO);

	/*
	 * Missed one or more clock interrupts, so let's start
	 * counting again from the current value.
	 */
	if ((ci->ci_next_cp0_clk_intr - new_cnt) & 0x80000000) {

		ci->ci_next_cp0_clk_intr = new_cnt + curcpu()->ci_cycles_per_hz;
		writereg(JZ_OST_DATA, ci->ci_next_cp0_clk_intr);
		curcpu()->ci_ev_count_compare_missed.ev_count++;
	}
	writereg(JZ_TC_TFCR, TFR_OSTFLAG);
#else
	writereg(JZ_TC_TFCR, TFR_FFLAG5);
#endif

#ifdef INGENIC_CLOCK_DEBUG
	cnt++;
	if (cnt == 100) {
		cnt = 0;
		ingenic_puts("+");
	}
#endif
#ifdef MULTIPROCESSOR
	/*
	 * XXX
	 * needs to take the IPI lock and ping all online CPUs, not just core 1
	 */
	MTC0(1 << IPI_CLOCK, 20, 1);
#endif
	hardclock(cf);
	splx(s);
}
