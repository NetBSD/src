/*	$NetBSD: clock_pcc.c,v 1.16.64.2 2008/01/19 12:14:24 bouyer Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * Glue for the Peripheral Channel Controller timers and the
 * Mostek clock chip found on the MVME-147.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock_pcc.c,v 1.16.64.2 2008/01/19 12:14:24 bouyer Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <machine/psl.h>
#include <machine/bus.h>

#include <dev/mvme/clockvar.h>

#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>

#include "ioconf.h"

int clock_pcc_match(struct device *, struct cfdata *, void *);
void clock_pcc_attach(struct device *, struct device *, void *);

struct clock_pcc_softc {
	struct device sc_dev;
	struct clock_attach_args sc_clock_args;
	u_char sc_clock_lvl;
	struct timecounter sc_tc;
};

CFATTACH_DECL(clock_pcc, sizeof(struct clock_pcc_softc),
    clock_pcc_match, clock_pcc_attach, NULL, NULL);


static int clock_pcc_profintr(void *);
static int clock_pcc_statintr(void *);
static void clock_pcc_initclocks(void *, int, int);
static u_int clock_pcc_getcount(struct timecounter *);
static void clock_pcc_shutdown(void *);

static struct clock_pcc_softc *clock_pcc_sc;
static uint32_t clock_pcc_count;
static uint16_t clock_pcc_reload;

/* ARGSUSED */
int
clock_pcc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pcc_attach_args *pa;

	pa = aux;

	/* Only one clock, please. */
	if (clock_pcc_sc)
		return 0;

	if (strcmp(pa->pa_name, clock_cd.cd_name))
		return 0;

	pa->pa_ipl = cf->pcccf_ipl;

	return 1;
}

/* ARGSUSED */
void
clock_pcc_attach(struct device *parent, struct device *self, void *aux)
{
	struct pcc_attach_args *pa;
	struct clock_pcc_softc *sc;

	sc = (struct clock_pcc_softc *)self;
	pa = aux;

	if (pa->pa_ipl != CLOCK_LEVEL)
		panic("clock_pcc_attach: wrong interrupt level");

	clock_pcc_sc = sc;
	sc->sc_clock_args.ca_arg = sc;
	sc->sc_clock_args.ca_initfunc = clock_pcc_initclocks;

	/* Do common portions of clock config. */
	clock_config(self, &sc->sc_clock_args, pccintr_evcnt(pa->pa_ipl));

	/* Ensure our interrupts get disabled at shutdown time. */
	(void)shutdownhook_establish(clock_pcc_shutdown, NULL);

	/* Attach the interrupt handlers. */
	pccintr_establish(PCCV_TIMER1, clock_pcc_profintr, pa->pa_ipl,
	    NULL, &clock_profcnt);
	pccintr_establish(PCCV_TIMER2, clock_pcc_statintr, pa->pa_ipl,
	    NULL, &clock_statcnt);
	sc->sc_clock_lvl = pa->pa_ipl | PCC_IENABLE | PCC_TIMERACK;
}

void
clock_pcc_initclocks(void *arg, int prof_us, int stat_us)
{
	struct clock_pcc_softc *sc = arg;

	clock_pcc_reload = pcc_timer_us2lim(prof_us);
	pcc_reg_write16(sys_pcc, PCCREG_TMR1_PRELOAD, clock_pcc_reload);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERSTART);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_INTR_CTRL, sc->sc_clock_lvl);

	pcc_reg_write16(sys_pcc, PCCREG_TMR2_PRELOAD,
	    pcc_timer_us2lim(stat_us));
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERSTART);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_INTR_CTRL, sc->sc_clock_lvl);

	sc->sc_tc.tc_get_timecount = clock_pcc_getcount;
	sc->sc_tc.tc_name = "pcc_count";
	sc->sc_tc.tc_frequency = PCC_TIMERFREQ;
	sc->sc_tc.tc_quality = 100;
	sc->sc_tc.tc_counter_mask = ~0;
	tc_init(&sc->sc_tc);
}

/* ARGSUSED */
u_int
clock_pcc_getcount(struct timecounter *tc)
{
	u_int cnt;
	uint16_t tc1, tc2;
	uint8_t cr;
	int s;

	s = splhigh();

	/*
	 * There's no way to latch the counter and overflow registers
	 * without pausing the clock, so compensate for the possible
	 * race by checking for counter wrap-around and re-reading the
	 * overflow counter if necessary.
	 *
	 * Note: This only works because we're at splhigh().
	 */
	tc1 = pcc_reg_read16(sys_pcc, PCCREG_TMR1_COUNT);
	cr = pcc_reg_read(sys_pcc, PCCREG_TMR1_CONTROL);
	tc2 = pcc_reg_read16(sys_pcc, PCCREG_TMR1_COUNT);
	if (tc1 > tc2) {
		cr = pcc_reg_read(sys_pcc, PCCREG_TMR1_CONTROL);
		tc1 = tc2;
	}
	cnt = clock_pcc_count;
	splx(s);
	/* XXX assume HZ == 100 */
	cnt += (tc1 - clock_pcc_reload) +
	    (PCC_TIMERFREQ / 100) * (cr >> PCC_TIMEROVFLSHIFT);

	return cnt;
}

int
clock_pcc_profintr(void *frame)
{
	uint8_t cr;
	uint16_t tc;
	int s;

	s = splhigh();
	tc = pcc_reg_read16(sys_pcc, PCCREG_TMR1_COUNT);
	cr = pcc_reg_read(sys_pcc, PCCREG_TMR1_CONTROL);
	if (tc > pcc_reg_read16(sys_pcc, PCCREG_TMR1_COUNT))
		cr = pcc_reg_read(sys_pcc, PCCREG_TMR1_CONTROL);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERSTART);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_INTR_CTRL,
	    clock_pcc_sc->sc_clock_lvl);
	splx(s);

	for (cr >>= PCC_TIMEROVFLSHIFT; cr; cr--) {
		/* XXX assume HZ == 100 */
		clock_pcc_count += PCC_TIMERFREQ / 100;
		hardclock(frame);
	}

	return 1;
}

int
clock_pcc_statintr(void *frame)
{

	/* Disable the timer interrupt while we handle it. */
	pcc_reg_write(sys_pcc, PCCREG_TMR2_INTR_CTRL, 0);

	statclock((struct clockframe *) frame);

	pcc_reg_write16(sys_pcc, PCCREG_TMR2_PRELOAD,
	    pcc_timer_us2lim(CLOCK_NEWINT(clock_statvar, clock_statmin)));
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERSTART);

	pcc_reg_write(sys_pcc, PCCREG_TMR2_INTR_CTRL,
	    clock_pcc_sc->sc_clock_lvl);

	return 1;
}

/* ARGSUSED */
void
clock_pcc_shutdown(void *arg)
{

	/* Make sure the timer interrupts are turned off. */
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_INTR_CTRL, 0);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_INTR_CTRL, 0);
}
