/*	$NetBSD: clock_pcctwo.c,v 1.5 2001/04/14 13:53:06 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 * Glue for the Peripheral Channel Controller Two (PCCChip2) timers,
 * the Memory Controller ASIC (MCchip, and the Mostek clock chip found
 * on the MVME-1[67]7 and MVME-162 series of boards.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/bus.h>

#include <mvme68k/mvme68k/clockreg.h>
#include <mvme68k/mvme68k/clockvar.h>
#include <mvme68k/dev/pcctwovar.h>
#include <mvme68k/dev/pcctworeg.h>


int clock_pcctwo_match __P((struct device *, struct cfdata *, void *));
void clock_pcctwo_attach __P((struct device *, struct device *, void *));

struct clock_pcctwo_softc {
	struct device sc_dev;
	struct clock_attach_args sc_clock_args;
	u_char sc_clock_lvl;
};

struct cfattach clock_pcctwo_ca = {
	sizeof(struct device), clock_pcctwo_match, clock_pcctwo_attach
};

extern struct cfdriver clock_cd;

static int clock_pcctwo_profintr __P((void *));
static int clock_pcctwo_statintr __P((void *));
static void clock_pcctwo_initclocks __P((void *, int, int));
static long clock_pcctwo_microtime __P((void *));
static void clock_pcctwo_shutdown __P((void *));

static struct clock_pcctwo_softc *clock_pcctwo_sc;

/* ARGSUSED */
int
clock_pcctwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcctwo_attach_args *pa = aux;

	/* Only one clock, please. */
	if (clock_pcctwo_sc)
		return (0);

	if (strcmp(pa->pa_name, clock_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcctwocf_ipl;

	return (1);
}

/* ARGSUSED */
void
clock_pcctwo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct clock_pcctwo_softc *sc;
	struct pcctwo_attach_args *pa;

	sc = clock_pcctwo_sc = (struct clock_pcctwo_softc *) self;
	pa = aux;

	if (pa->pa_ipl != CLOCK_LEVEL)
		panic("clock_pcctwo_attach: wrong interrupt level");

	/* Map the RTC's registers */
	sc->sc_clock_args.ca_bust = pa->pa_bust;
	bus_space_map(pa->pa_bust, pa->pa_offset,
	    MK48T_REGSIZE, 0, &sc->sc_clock_args.ca_bush);

	sc->sc_clock_args.ca_arg = sc;
	sc->sc_clock_args.ca_initfunc = clock_pcctwo_initclocks;
	sc->sc_clock_args.ca_microtime = clock_pcctwo_microtime;

	/* Do common portions of clock config. */
	clock_config(self, &sc->sc_clock_args);

	/* Ensure our interrupts get disabled at shutdown time. */
	(void) shutdownhook_establish(clock_pcctwo_shutdown, NULL);

	sc->sc_clock_lvl = (pa->pa_ipl & PCCTWO_ICR_LEVEL_MASK) |
	    PCCTWO_ICR_ICLR | PCCTWO_ICR_IEN;

	/* Attach the interrupt handlers. */
	pcctwointr_establish(PCCTWOV_TIMER1, clock_pcctwo_profintr,
	    pa->pa_ipl, NULL);
	pcctwointr_establish(PCCTWOV_TIMER2, clock_pcctwo_statintr,
	    pa->pa_ipl, NULL);
}

void
clock_pcctwo_initclocks(arg, proftick, stattick)
	void *arg;
	int proftick;
	int stattick;
{
	struct clock_pcctwo_softc *sc;

	sc = arg;

	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_CONTROL, PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write32(sys_pcctwo, PCC2REG_TIMER1_COUNTER, 0);
	pcc2_reg_write32(sys_pcctwo, PCC2REG_TIMER1_COMPARE,
	    PCCTWO_US2LIM(proftick));
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_CONTROL,
	    PCCTWO_TT_CTRL_CEN | PCCTWO_TT_CTRL_COC | PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_ICSR, sc->sc_clock_lvl);

	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_CONTROL, PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write32(sys_pcctwo, PCC2REG_TIMER2_COUNTER, 0);
	pcc2_reg_write32(sys_pcctwo, PCC2REG_TIMER2_COMPARE,
	    PCCTWO_US2LIM(stattick));
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_CONTROL,
	    PCCTWO_TT_CTRL_CEN | PCCTWO_TT_CTRL_COC | PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_ICSR, sc->sc_clock_lvl);
}

/* ARGSUSED */
long
clock_pcctwo_microtime(arg)
	void *arg;
{
	static int ovfl_adj[] = {
		0,       10000,  20000,  30000,
		40000,   50000,  60000,  70000,
		80000,   90000, 100000, 110000,
		120000, 130000, 140000, 150000};
	u_int8_t cr;
	u_int32_t tc, tc2;

	/*
	 * There's no way to latch the counter and overflow registers
	 * without pausing the clock, so compensate for the possible
	 * race by checking for counter wrap-around and re-reading the
	 * overflow counter if necessary.
	 *
	 * Note: This only works because we're called at splhigh().
	 */
	tc = pcc2_reg_read32(sys_pcctwo, PCC2REG_TIMER1_COUNTER);
	cr = pcc2_reg_read(sys_pcctwo, PCC2REG_TIMER1_CONTROL);
	if (tc > (tc2 = pcc2_reg_read32(sys_pcctwo, PCC2REG_TIMER1_COUNTER))) {
		cr = pcc2_reg_read(sys_pcctwo, PCC2REG_TIMER1_CONTROL);
		tc = tc2;
	}

	return ((long) PCCTWO_LIM2US(tc) + ovfl_adj[PCCTWO_TT_CTRL_OVF(cr)]);
}

int
clock_pcctwo_profintr(frame)
	void *frame;
{
	u_int8_t cr;
	u_int32_t tc;
	int s;

	s = splhigh();
	tc = pcc2_reg_read32(sys_pcctwo, PCC2REG_TIMER1_COUNTER);
	cr = pcc2_reg_read(sys_pcctwo, PCC2REG_TIMER1_CONTROL);
	if (tc > pcc2_reg_read32(sys_pcctwo, PCC2REG_TIMER1_COUNTER))
		cr = pcc2_reg_read(sys_pcctwo, PCC2REG_TIMER1_CONTROL);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_CONTROL,
	    PCCTWO_TT_CTRL_CEN | PCCTWO_TT_CTRL_COC | PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_ICSR,
	    clock_pcctwo_sc->sc_clock_lvl);
	__asm __volatile("movw %0,%%sr" : : "di" (s));

	for (cr = PCCTWO_TT_CTRL_OVF(cr); cr; cr--) {
		hardclock(frame);
		clock_profcnt.ev_count++;
	}

	return (1);
}

int
clock_pcctwo_statintr(frame)
	void *frame;
{

	/* Disable the timer interrupt while we handle it. */
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_ICSR, 0);

	statclock((struct clockframe *) frame);

	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_CONTROL, PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write32(sys_pcctwo, PCC2REG_TIMER2_COUNTER, 0);
	pcc2_reg_write32(sys_pcctwo, PCC2REG_TIMER2_COMPARE,
	    PCCTWO_US2LIM(CLOCK_NEWINT(clock_statvar, clock_statmin)));
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_CONTROL,
	    PCCTWO_TT_CTRL_CEN | PCCTWO_TT_CTRL_COC | PCCTWO_TT_CTRL_COVF);

	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_ICSR,
	    clock_pcctwo_sc->sc_clock_lvl);

	clock_statcnt.ev_count++;
	return (1);
}

/* ARGSUSED */
void
clock_pcctwo_shutdown(arg)
	void *arg;
{

	/* Make sure the timer interrupts are turned off. */
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_CONTROL, PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER1_ICSR, 0);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_CONTROL, PCCTWO_TT_CTRL_COVF);
	pcc2_reg_write(sys_pcctwo, PCC2REG_TIMER2_ICSR, 0);
}
