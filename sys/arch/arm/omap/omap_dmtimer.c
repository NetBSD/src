/*	$NetBSD: omap_dmtimer.c,v 1.1.2.2 2013/01/16 05:32:49 yamt Exp $	*/

/*
 * TI OMAP Dual-mode timers
 */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: omap_dmtimer.c,v 1.1.2.2 2013/01/16 05:32:49 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>

#include <machine/intr.h>

#include <arm/omap/omap2_prcm.h>
#include <arm/omap/omap_dmtimerreg.h>
#include <arm/omap/omap_dmtimervar.h>

typedef uint8_t dmt_reg_t;
typedef uint16_t dmt_timer_reg_t;

static unsigned int
		dmt_tc_get_timecount(struct timecounter *);
static int	dmt_hardintr(void *);
static int	dmt_statintr(void *);
static void	dmt_start_periodic_intr(struct omap_dmtimer_softc *, int,
		    unsigned int, int (*)(void *));
static void	dmt_set_periodic_intr_frequency(struct omap_dmtimer_softc *,
		    unsigned int);
static void	dmt_start_timecounter(struct omap_dmtimer_softc *);
static unsigned int
		dmt_get_timecount(struct omap_dmtimer_softc *);
static void	dmt_start(struct omap_dmtimer_softc *, unsigned int);
static void	dmt_reset(struct omap_dmtimer_softc *);
static void	dmt_enable(struct omap_dmtimer_softc *);
static void	dmt_intr_enable(struct omap_dmtimer_softc *, uint32_t);
static void	dmt_intr_ack(struct omap_dmtimer_softc *, uint32_t);
static uint32_t	dmt_timer_read_4(struct omap_dmtimer_softc *, dmt_timer_reg_t);
static void	dmt_timer_write_4(struct omap_dmtimer_softc *, dmt_timer_reg_t,
		    uint32_t);
static void	dmt_timer_write_post_wait(struct omap_dmtimer_softc *,
		    dmt_timer_reg_t);
static uint32_t	dmt_read_4(struct omap_dmtimer_softc *, dmt_reg_t);
static void	dmt_write_4(struct omap_dmtimer_softc *, dmt_reg_t, uint32_t);

static struct omap_dmtimer_softc *hardclock_sc;
static struct omap_dmtimer_softc *statclock_sc;
static struct timecounter dmt_timecounter;

void
omap_dmtimer_attach_timecounter(struct omap_dmtimer_softc *sc)
{

	if (dmt_timecounter.tc_priv != NULL)
		panic("omap dmtimer timecounter already initialized");

	dmt_timecounter.tc_priv = sc;
}

static struct timecounter dmt_timecounter = {
	.tc_get_timecount	= dmt_tc_get_timecount,
	.tc_counter_mask	= 0xffffffff, /* XXXMAGIC Make sense?  */
	.tc_frequency		= OMAP_SYSTEM_CLOCK_FREQ, /* XXXPOWER */
	.tc_name		= "dmtimer", /* XXX Which one?  */
	.tc_quality		= 100, /* XXXMAGIC?  */
	.tc_priv		= NULL,
};

static unsigned int
dmt_tc_get_timecount(struct timecounter *tc)
{
	struct omap_dmtimer_softc *sc = tc->tc_priv;

	if (sc == NULL)
		panic("uninitialized omap dmtimer timecounter");

	return dmt_get_timecount(sc);
}

void
omap_dmtimer_attach_hardclock(struct omap_dmtimer_softc *sc)
{

	if (hardclock_sc != NULL)
		panic("%s: replacing hardclock %s", device_xname(sc->sc_dev),
		    device_xname(hardclock_sc->sc_dev));
	hardclock_sc = sc;
}

void
omap_dmtimer_attach_statclock(struct omap_dmtimer_softc *sc)
{

	KASSERT(stathz != 0);
	if (statclock_sc != NULL)
		panic("%s: replacing statclock %s", device_xname(sc->sc_dev),
		    device_xname(statclock_sc->sc_dev));
	statclock_sc = sc;
}

void
cpu_initclocks(void)
{
	struct omap_dmtimer_softc *timecounter_sc = dmt_timecounter.tc_priv;

	if (hardclock_sc == NULL)
		panic("omap dmtimer hardclock not initialized");
	dmt_enable(hardclock_sc);
	dmt_start_periodic_intr(hardclock_sc, IPL_CLOCK, hz, &dmt_hardintr);

	if (timecounter_sc == NULL)
		panic("omap dmtimer timecounter not initialized");
	dmt_enable(statclock_sc);
	dmt_start_periodic_intr(statclock_sc, IPL_HIGH, stathz, &dmt_statintr);

	if (statclock_sc == NULL)
		panic("omap dmtimer statclock not initialized");
	dmt_enable(timecounter_sc);
	dmt_start_timecounter(timecounter_sc);
	tc_init(&dmt_timecounter);
}

void
setstatclockrate(int rate)
{
	struct omap_dmtimer_softc *sc = statclock_sc;

	if (rate < 0)
		panic("I can't run the statistics clock backward!");

	if (sc == NULL)
		panic("There is no statclock timer!\n");

	dmt_set_periodic_intr_frequency(sc, rate);
}

static int
dmt_hardintr(void *frame)
{
	struct omap_dmtimer_softc *sc = hardclock_sc;

	KASSERT(sc != NULL);
	dmt_intr_ack(sc, OMAP_DMTIMER_INTR_ALL);
	hardclock(frame);

	return 1;
}

static int
dmt_statintr(void *frame)
{
	struct omap_dmtimer_softc *sc = statclock_sc;

	KASSERT(sc != NULL);
	dmt_intr_ack(sc, OMAP_DMTIMER_INTR_ALL);
	statclock(frame);

	return 1;
}

static void
dmt_start_periodic_intr(struct omap_dmtimer_softc *sc, int ipl,
    unsigned int frequency, int (*func)(void *))
{

	dmt_reset(sc);
	dmt_start(sc, frequency);
	/*
	 * Null argument means func gets the interrupt frame.  For
	 * whatever reason it's not an option to pass an argument (such
	 * as sc) and the interrupt frame both, which is why we have
	 * the global hardclock_sc and statclock_sc.
	 */
	intr_establish(sc->sc_intr, ipl, IST_LEVEL, func, 0);
	dmt_intr_enable(sc, OMAP_DMTIMER_INTR_OVERFLOW);
}

static void
dmt_set_periodic_intr_frequency(struct omap_dmtimer_softc *sc,
    unsigned int frequency)
{

	dmt_reset(sc);
	dmt_start(sc, frequency);
	dmt_intr_enable(sc, OMAP_DMTIMER_INTR_OVERFLOW);
}

static void
dmt_start_timecounter(struct omap_dmtimer_softc *sc)
{

	/*
	 * XXXPOWER On reset, the timer uses the system clock.  For
	 * low-power operation, we can configure timers to use less
	 * frequent clocks, but there's currently no abstraction for
	 * doing this.
	 */
	dmt_reset(sc);
	dmt_timer_write_4(sc, OMAP_DMTIMER_TIMER_LOAD, 0);
	dmt_timer_write_4(sc, OMAP_DMTIMER_TIMER_COUNTER, 0);
	dmt_timer_write_4(sc, OMAP_DMTIMER_TIMER_CTRL,
	    (OMAP_DMTIMER_TIMER_CTRL_START |
		OMAP_DMTIMER_TIMER_CTRL_AUTORELOAD));
}

static unsigned int
dmt_get_timecount(struct omap_dmtimer_softc *sc)
{

	return dmt_timer_read_4(sc, OMAP_DMTIMER_TIMER_COUNTER);
}

static void
dmt_start(struct omap_dmtimer_softc *sc, unsigned int frequency)
{
	uint32_t value;

	/*
	 * XXXPOWER Should do something clever with prescaling and
	 * clock selection to save power.
	 */

	/*
	 * XXX The dmtimer doesn't even necessarily run at the system
	 * clock frequency by default.  On the AM335x, the system clock
	 * frequency is 24 MHz, but dmtimer0 runs at 32 kHz.
	 */
	value = (0xffffffff - ((OMAP_SYSTEM_CLOCK_FREQ / frequency) - 1));

	dmt_timer_write_4(sc, OMAP_DMTIMER_TIMER_LOAD, value);
	dmt_timer_write_4(sc, OMAP_DMTIMER_TIMER_COUNTER, value);

	dmt_timer_write_4(sc, OMAP_DMTIMER_TIMER_CTRL,
	    (OMAP_DMTIMER_TIMER_CTRL_START |
		OMAP_DMTIMER_TIMER_CTRL_AUTORELOAD));
}

static void
dmt_reset(struct omap_dmtimer_softc *sc)
{
	uint32_t reset_mask;
	unsigned int tries = 1000; /* XXXMAGIC */

	if (sc->sc_version == 1)
		reset_mask = OMAP_DMTIMER_V1_OCP_CFG_SOFTRESET_MASK;
	else
		reset_mask = OMAP_DMTIMER_V2_OCP_CFG_SOFTRESET_MASK;

	dmt_write_4(sc, OMAP_DMTIMER_OCP_CFG, reset_mask);
	while (dmt_read_4(sc, OMAP_DMTIMER_OCP_CFG) & reset_mask) {
		if (--tries == 0)
			panic("unable to reset dmtimer %p", sc);
		DELAY(10);	/* XXXMAGIC */
	}

	/*
	 * Posted mode is enabled on reset on the OMAP35x but disabled
	 * on reset on the AM335x, so handle both cases.
	 *
	 * XXXPOWER Does enabling this reduce power consumption?
	 */
	sc->sc_posted =
	    (0 != (dmt_read_4(sc, OMAP_DMTIMER_TIMER_SYNC_INT_CTRL)
		& OMAP_DMTIMER_TIMER_SYNC_INT_CTRL_POSTED));
}

static void
dmt_enable(struct omap_dmtimer_softc *sc)
{

	if (!sc->sc_enabled) {
		prcm_module_enable(sc->sc_module);
		sc->sc_enabled = 1;
	}
}

static void
dmt_intr_enable(struct omap_dmtimer_softc *sc, uint32_t intr)
{

	if (sc->sc_version == 1) {
		dmt_write_4(sc, OMAP_DMTIMER_V1_INTR_ENABLE, intr);
	} else {
		dmt_write_4(sc, OMAP_DMTIMER_V2_INTR_ENABLE_CLEAR,
		    (OMAP_DMTIMER_INTR_ALL &~ intr));
		dmt_write_4(sc, OMAP_DMTIMER_V2_INTR_ENABLE_SET, intr);
	}
}

static void
dmt_intr_ack(struct omap_dmtimer_softc *sc, uint32_t intr)
{

	if (sc->sc_version == 1)
		dmt_write_4(sc, OMAP_DMTIMER_V1_INTR_STATUS, intr);
	else
		dmt_write_4(sc, OMAP_DMTIMER_V2_INTR_STATUS, intr);
}

static uint32_t
dmt_timer_read_4(struct omap_dmtimer_softc *sc, dmt_timer_reg_t reg)
{
	dmt_reg_t timer_base;

	if (sc->sc_version == 1)
		timer_base = OMAP_DMTIMER_V1_TIMER_REGS;
	else
		timer_base = OMAP_DMTIMER_V2_TIMER_REGS;

	dmt_timer_write_post_wait(sc, reg);
	return dmt_read_4(sc, (timer_base + reg));
}

static void
dmt_timer_write_4(struct omap_dmtimer_softc *sc, dmt_timer_reg_t reg,
    uint32_t value)
{
	dmt_reg_t timer_base;

	if (sc->sc_version == 1)
		timer_base = OMAP_DMTIMER_V1_TIMER_REGS;
	else
		timer_base = OMAP_DMTIMER_V2_TIMER_REGS;

	dmt_timer_write_post_wait(sc, reg);
	dmt_write_4(sc, (timer_base + reg), value);
}

static void
dmt_timer_write_post_wait(struct omap_dmtimer_softc *sc, dmt_timer_reg_t reg)
{
	dmt_reg_t timer_base;

	if (sc->sc_version == 1)
		timer_base = OMAP_DMTIMER_V1_TIMER_REGS;
	else
		timer_base = OMAP_DMTIMER_V2_TIMER_REGS;

	/*
	 * Make sure we can read the TWPS (OMAP_DMTIMER_TIMER_WRITE_POST)
	 * register with vanilla dmt_read_4.
	 */
	CTASSERT(OMAP_DMTIMER_TIMER_WRITE_POST ==
	    OMAP_DMTIMER_REG_POSTED_INDEX(OMAP_DMTIMER_TIMER_WRITE_POST));

	if (sc->sc_posted && OMAP_DMTIMER_REG_POSTED_P(reg)) {
		unsigned int tries = 1000; /* XXXMAGIC */
		const dmt_reg_t write_post_reg = (timer_base +
		    OMAP_DMTIMER_TIMER_WRITE_POST);

		while (dmt_read_4(sc, write_post_reg) &
		    OMAP_DMTIMER_REG_POSTED_MASK(reg)) {
			if (--tries == 0)
				panic("dmtimer %p failed to complete write",
				    sc);
			DELAY(10);
		}
	}
}

static uint32_t
dmt_read_4(struct omap_dmtimer_softc *sc, dmt_reg_t reg)
{

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg);
}

static void
dmt_write_4(struct omap_dmtimer_softc *sc, dmt_reg_t reg, uint32_t value)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, value);
}
