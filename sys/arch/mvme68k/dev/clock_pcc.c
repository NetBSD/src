/*	$NetBSD: clock_pcc.c,v 1.10 2001/08/12 18:33:13 scw Exp $	*/

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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/bus.h>

#include <mvme68k/mvme68k/clockvar.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>

int clock_pcc_match __P((struct device *, struct cfdata *, void *));
void clock_pcc_attach __P((struct device *, struct device *, void *));

struct clock_pcc_softc {
	struct device sc_dev;
	struct clock_attach_args sc_clock_args;
	u_char sc_clock_lvl;
};

struct cfattach clock_pcc_ca = {
	sizeof(struct clock_pcc_softc), clock_pcc_match, clock_pcc_attach
};

extern struct cfdriver clock_cd;


static int clock_pcc_profintr __P((void *));
static int clock_pcc_statintr __P((void *));
static void clock_pcc_initclocks __P((void *, int, int));
static long clock_pcc_microtime __P((void *));
static void clock_pcc_shutdown __P((void *));

static struct clock_pcc_softc *clock_pcc_sc;

/* ARGSUSED */
int
clock_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcc_attach_args *pa;

	pa = aux;

	/* Only one clock, please. */
	if (clock_pcc_sc)
		return (0);

	if (strcmp(pa->pa_name, clock_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcccf_ipl;

	return (1);
}

/* ARGSUSED */
void
clock_pcc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pcc_attach_args *pa;
	struct clock_pcc_softc *sc;

	sc = (struct clock_pcc_softc *) self;
	pa = aux;

	if (pa->pa_ipl != CLOCK_LEVEL)
		panic("clock_pcc_attach: wrong interrupt level");

	clock_pcc_sc = sc;
	sc->sc_clock_args.ca_arg = sc;
	sc->sc_clock_args.ca_initfunc = clock_pcc_initclocks;
	sc->sc_clock_args.ca_microtime = clock_pcc_microtime;

	/* Do common portions of clock config. */
	clock_config(self, &sc->sc_clock_args, pccintr_evcnt(pa->pa_ipl));

	/* Ensure our interrupts get disabled at shutdown time. */
	(void) shutdownhook_establish(clock_pcc_shutdown, NULL);

	/* Attach the interrupt handlers. */
	pccintr_establish(PCCV_TIMER1, clock_pcc_profintr, pa->pa_ipl,
	    NULL, &clock_profcnt);
	pccintr_establish(PCCV_TIMER2, clock_pcc_statintr, pa->pa_ipl,
	    NULL, &clock_statcnt);
	sc->sc_clock_lvl = pa->pa_ipl | PCC_IENABLE | PCC_TIMERACK;
}

void
clock_pcc_initclocks(arg, proftick, stattick)
	void *arg;
	int proftick;
	int stattick;
{
	struct clock_pcc_softc *sc = arg;

	pcc_reg_write16(sys_pcc, PCCREG_TMR1_PRELOAD,
	    pcc_timer_us2lim(proftick));
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERSTART);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_INTR_CTRL, sc->sc_clock_lvl);

	pcc_reg_write16(sys_pcc, PCCREG_TMR2_PRELOAD,
	    pcc_timer_us2lim(stattick));
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERSTART);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_INTR_CTRL, sc->sc_clock_lvl);
}

/* ARGSUSED */
long
clock_pcc_microtime(arg)
	void *arg;
{
	static int ovfl_adj[] = {
		0,       10000,  20000,  30000,
		40000,   50000,  60000,  70000,
		80000,   90000, 100000, 110000,
		120000, 130000, 140000, 150000};
	u_int8_t cr;
	u_int16_t tc, tc2;

	/*
	 * There's no way to latch the counter and overflow registers
	 * without pausing the clock, so compensate for the possible
	 * race by checking for counter wrap-around and re-reading the
	 * overflow counter if necessary.
	 *
	 * Note: This only works because we're called at splhigh().
	 */
	tc = pcc_reg_read16(sys_pcc, PCCREG_TMR1_COUNT);
	cr = pcc_reg_read(sys_pcc, PCCREG_TMR1_CONTROL);
	if (tc > (tc2 = pcc_reg_read16(sys_pcc, PCCREG_TMR1_COUNT))) {
		cr = pcc_reg_read(sys_pcc, PCCREG_TMR1_CONTROL);
		tc = tc2;
	}

	return ((long) pcc_timer_cnt2us(tc) + ovfl_adj[cr>>PCC_TIMEROVFLSHIFT]);
}

int
clock_pcc_profintr(frame)
	void *frame;
{
	u_int8_t cr;
	u_int16_t tc;
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

	for (cr >>= PCC_TIMEROVFLSHIFT; cr; cr--)
		hardclock(frame);

	return (1);
}

int
clock_pcc_statintr(frame)
	void *frame;
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

	return (1);
}

/* ARGSUSED */
void
clock_pcc_shutdown(arg)
	void *arg;
{

	/* Make sure the timer interrupts are turned off. */
	pcc_reg_write(sys_pcc, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR1_INTR_CTRL, 0);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_CONTROL, PCC_TIMERCLEAR);
	pcc_reg_write(sys_pcc, PCCREG_TMR2_INTR_CTRL, 0);
}
