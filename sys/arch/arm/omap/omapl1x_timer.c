
/*
 * Copyright (c) 2013 Sughosh Ganu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: omapl1x_timer.c,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $");

#include "opt_timer.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <machine/intr.h>

#include <arm/cpufunc.h>
#include <arm/pic/picvar.h>

#include <arm/omap/omap_tipb.h>
#include <arm/omap/omapl1x_reg.h>
#include <arm/omap/omapl1x_misc.h>

typedef struct timer_factors {
	uint32_t tf_counts_per_usec;
	uint32_t tf_period;
	uint32_t tf_enamode;
	uint32_t tf_ctr_reg;
	uint32_t tf_prd_reg;
	uint32_t tf_enamode_shift;
	uint32_t tf_intr_prd_en_shift;
	uint32_t tf_intr_prd_stat_shift;
} timer_factors_t;

typedef struct omapl1xtmr_softc {
	struct device		sc_dev;
	uint			sc_timerno;
	uint			sc_timer_freq;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_addr;
	size_t			sc_size;
	int			sc_intr;
	timer_factors_t		sc_tf;
	uint			sc_bot;
} omapl1xtmr_softc_t;

static struct omapl1x_wdt {
	bus_space_tag_t		wdt_iot;	/* Bus tag */
	bus_addr_t		wdt_addr;	/* Address */
	bus_space_handle_t	wdt_ioh;
	bus_size_t		wdt_size;
} wdt;

static int  omapl1xtimer_match(device_t, struct cfdata *, void *);
static void omapl1xtimer_attach(device_t, device_t, void *);

static int omapl1xtimer_clockintr(void *frame);
static int omapl1xtimer_statintr(void *frame);
static void omapl1x_microtime_init(void);
static inline uint32_t omapl1x_get_timecount(struct timecounter *tc);
static inline void omapl1xtimer_stop(struct omapl1xtmr_softc *sc);
static inline uint32_t omapl1xtimer_read(struct omapl1xtmr_softc *sc);
static void omapl1xtimer_prd_intr_dis(struct omapl1xtmr_softc *sc);
static void omapl1xtimer_prd_intr_enb(struct omapl1xtmr_softc *sc);
static void omapl1xtimer_prd_intr_clr(struct omapl1xtmr_softc *sc);
static void omapl1xtimer_start(struct omapl1xtmr_softc *sc);
static void timer_factors(struct omapl1xtmr_softc *sc, int ints_per_sec,
			  uint8_t enamode);
static void timer_init(struct omapl1xtmr_softc *sc, int schz, uint8_t enamode,
		       boolean_t intr);

static struct timecounter omapl1x_timecounter = {
	.tc_get_timecount = omapl1x_get_timecount,
	.tc_counter_mask = 0xffffffff,
	.tc_name = "gpt",
	.tc_quality = 100,
	.tc_priv = NULL
};

#ifdef OMAPL1X_TIMER_DEBUG
static void tfprint(uint, timer_factors_t *);
#endif

static uint32_t counts_per_usec;
static uint32_t counts_per_hz = ~0;
static struct omapl1xtmr_softc *clock_sc;
static struct omapl1xtmr_softc *stat_sc;
static struct omapl1xtmr_softc *ref_sc;

/* Timer modes */
#define TGCR_TIMMODE_64BIT		0x0
#define TGCR_TIMMODE_32BIT_UNCHANINED	0x1
#define TGCR_TIMMODE_64BIT_WDOG		0x2
#define TGCR_TIMMODE_32BIT_CHANINED	0x3
#define TGCR_TIMMODE_SHIFT		2

#define TGCR_RS_STOP		0x0
#define TGCR_RS_RUN		0x1
#define TGCR_RS_MASK		0x3
#define TGCR_RS_12_SHIFT	0
#define TGCR_RS_34_SHIFT	1

#define TCR_ENAMODE_DISABLE	0x0
#define TCR_ENAMODE_ONESHOT	0x1
#define TCR_ENAMODE_CONTINUOUS	0x2
#define TCR_ENAMODE_RELOAD	0x3
#define TCR_ENAMODE_MASK	0x3
#define TCR_ENAMODE_12_SHIFT	6
#define TCR_ENAMODE_34_SHIFT	22

#define INTR_PRD_12_EN_SHIFT	0
#define INTR_PRD_34_EN_SHIFT	16
#define INTR_PRD_12_STAT_SHIFT	1
#define INTR_PRD_34_STAT_SHIFT	17

/* Watchdog related macros */
#define WDTCR_WDTKEY1		0xA5C6
#define WDTCR_WDTKEY2		0xDA7E

#define WDTCR_WDKEY_SHIFT	16
#define WDTCR_WDEN_SHIFT	14
#define WDTCR_WDKEY_MASK	0xffff0000

CFATTACH_DECL_NEW(omapl1xtimer, sizeof(struct omapl1xtmr_softc),
    omapl1xtimer_match, omapl1xtimer_attach, NULL, NULL);

static void
omapl1xtimer_prd_intr_dis (struct omapl1xtmr_softc *sc)
{
	uint32_t val;
	timer_factors_t *tfp = &sc->sc_tf;

	val  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, INTCTLSTAT);
	val &= ~(1 << tfp->tf_intr_prd_en_shift);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTCTLSTAT, val);
}

static void
omapl1xtimer_prd_intr_enb (struct omapl1xtmr_softc *sc)
{
	uint32_t val;
	timer_factors_t *tfp = &sc->sc_tf;

	val  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, INTCTLSTAT);
	val |= 1 << tfp->tf_intr_prd_en_shift;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTCTLSTAT, val);
}

static void
omapl1xtimer_prd_intr_clr (struct omapl1xtmr_softc *sc)
{
	uint32_t val;
	timer_factors_t *tfp = &sc->sc_tf;

	val  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, INTCTLSTAT);
	val |= 1 << tfp->tf_intr_prd_stat_shift;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTCTLSTAT, val);
}

static inline uint32_t
omapl1xtimer_read (struct omapl1xtmr_softc *sc)
{
	timer_factors_t *tfp = &sc->sc_tf;

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, tfp->tf_ctr_reg);
}

static inline void
omapl1xtimer_stop (struct omapl1xtmr_softc *sc)
{
	uint32_t val;
	timer_factors_t *tfp = &sc->sc_tf;

	val  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, TCR);
	val &= ~(TCR_ENAMODE_MASK << tfp->tf_enamode_shift);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TCR, val);
}

static void
omapl1xtimer_start (struct omapl1xtmr_softc *sc)
{
	uint32_t val, shift;
	timer_factors_t *tfp = &sc->sc_tf;

	/* get the timer to be used out of reset */
	shift = sc->sc_bot ? TGCR_RS_12_SHIFT : TGCR_RS_34_SHIFT;
	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, TGCR);

	val |= TGCR_RS_RUN << shift;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TGCR, val);

	/* set the desired timer period */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, tfp->tf_prd_reg, 
			  tfp->tf_period);

	/* set the selected enamode to get the timer running */
	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, TCR);

	val |= tfp->tf_enamode << tfp->tf_enamode_shift;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TCR, val);
}

static inline uint32_t
omapl1x_get_timecount (struct timecounter *tc)
{
	return  omapl1xtimer_read(ref_sc);
}

int
omapl1xtimer_clockintr (void *frame)
{
	struct omapl1xtmr_softc *sc = clock_sc;

	omapl1xtimer_prd_intr_clr(sc);
	hardclock(frame);
	return 1;
}

int
omapl1xtimer_statintr (void *frame)
{
	struct omapl1xtmr_softc *sc = stat_sc;

	omapl1xtimer_prd_intr_clr(sc);
	statclock(frame);
	return 1;
}

static void
timer_init (struct omapl1xtmr_softc *sc, int schz, uint8_t enamode,
	   boolean_t intr)
{
	int val = 0;

	timer_factors(sc, schz, enamode);

	omapl1xtimer_stop(sc);
	omapl1xtimer_prd_intr_dis(sc);
	omapl1xtimer_prd_intr_clr(sc);

	/* Clear tcr, tgcr, timer counters and period registers */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TCR, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TGCR, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIM12, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIM34, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PRD12, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PRD34, 0);

	if (intr)
		omapl1xtimer_prd_intr_enb(sc);

	/* Set timers to 32 bot unchained mode */
	val = TGCR_TIMMODE_32BIT_UNCHANINED << TGCR_TIMMODE_SHIFT;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TGCR, val);

	omapl1xtimer_start(sc);
}

static void
omapl1x_microtime_init (void)
{
	if (ref_sc == NULL)
		panic("microtime reference timer was not configured.");
	timer_init(ref_sc, 0, TCR_ENAMODE_CONTINUOUS, FALSE);
}

void
setstatclockrate (int schz)
{
	if (stat_sc == NULL)
		panic("Statistics timer was not configured.");
	timer_init(stat_sc, schz, TCR_ENAMODE_CONTINUOUS, TRUE);
}

/*
 * clock_sc and stat_sc starts here
 * ref_sc is initialized already by tipbtimer_attach
 */
void
cpu_initclocks(void)
{
	if (clock_sc == NULL)
		panic("Clock timer was not configured.");
	if (stat_sc == NULL)
		panic("Statistics timer was not configured.");
	if (ref_sc == NULL)
		panic("Microtime reference timer was not configured.");

	/*
	 * We already have the timers running, but not generating interrupts.
	 * In addition, we've set stathz and profhz.
	 */
	printf("clock: hz=%d stathz=%d\n", hz, stathz);

	/*
	 * The "cookie" parameter must be zero to pass the interrupt frame
	 * through to hardclock() and statclock().
	 */
	intr_establish(clock_sc->sc_intr, IPL_CLOCK, IST_LEVEL_HIGH,
		       omapl1xtimer_clockintr, 0);

	intr_establish(stat_sc->sc_intr, IPL_HIGH, IST_LEVEL_HIGH,
		       omapl1xtimer_statintr, 0);

	timer_init(clock_sc, hz, TCR_ENAMODE_CONTINUOUS, TRUE);
	timer_init(stat_sc, stathz, TCR_ENAMODE_CONTINUOUS, TRUE);

	omapl1x_timecounter.tc_frequency = omapl1x_get_tc_freq();

	tc_init(&omapl1x_timecounter);
}

void
delay (u_int n)
{
	struct omapl1xtmr_softc *sc = ref_sc;
	uint32_t cur, last, delta, usecs;

	if (sc == NULL)
		panic("The timer must be initialized sooner.");

	/*
	 * This works by polling the timer and counting the
	 * number of microseconds that go by.
	 */
	last = omapl1xtimer_read(sc);

	delta = usecs = 0;

	while (n > usecs) {
		cur = omapl1xtimer_read(sc);

		/* Check to see if the timer has wrapped around. */
		if (last > cur)
			delta += (cur + (counts_per_hz - last));
		else
			delta += (cur - last);

		last = cur;

		if (delta >= counts_per_usec) {
			usecs += delta / counts_per_usec;
			delta %= counts_per_usec;
		}
	}
}

static void
timer_factors (struct omapl1xtmr_softc *sc, int ints_per_sec, uint8_t enamode)
{
	timer_factors_t *tfp = &sc->sc_tf;
	const uint32_t us_per_sec = 1000000;

	if (ints_per_sec == 0) {
		tfp->tf_period = ~0U;
		counts_per_usec = sc->sc_timer_freq / us_per_sec;
	} else {
		uint32_t count_freq;
		count_freq = sc->sc_timer_freq;
		count_freq /= ints_per_sec;
		tfp->tf_period = count_freq;
	}

	tfp->tf_counts_per_usec = sc->sc_timer_freq / us_per_sec;
	tfp->tf_enamode = enamode;

	if (sc->sc_bot) {
		tfp->tf_ctr_reg = TIM12;
		tfp->tf_prd_reg = PRD12;
		tfp->tf_enamode_shift = TCR_ENAMODE_12_SHIFT;
		tfp->tf_intr_prd_en_shift = INTR_PRD_12_EN_SHIFT;
		tfp->tf_intr_prd_stat_shift = INTR_PRD_12_STAT_SHIFT;
	} else {
		tfp->tf_ctr_reg = TIM34;
		tfp->tf_prd_reg = PRD34;
		tfp->tf_enamode_shift = TCR_ENAMODE_34_SHIFT;
		tfp->tf_intr_prd_en_shift = INTR_PRD_34_EN_SHIFT;
		tfp->tf_intr_prd_stat_shift = INTR_PRD_34_STAT_SHIFT;
	}

#ifdef OMAPL1X_TIMER_DEBUG
	tfprint(sc->sc_timerno, tfp);
	Debugger();
#endif
}

#ifdef OMAPL1X_TIMER_DEBUG
void
tfprint (uint n, timer_factors_t *tfp)
{
	printf("%s: timer# %d\n", __func__, n);
	printf("\ttf_counts_per_usec: %#x\n", tfp->tf_counts_per_usec);
	printf("\ttf_counter: %#x\n", tfp->period);
	printf("\ttf_enamode: %#x\n", tfp->tf_enamode);
}
#endif

static int
omapl1xtimer_match (device_t parent, struct cfdata *match, void *aux)
{
	return 1;
}

void
omapl1xtimer_attach (device_t parent, device_t self, void *aux)
{
	struct omapl1xtmr_softc *sc = device_private(self);
	struct tipb_attach_args *tipb = aux;

	sc->sc_timerno = self->dv_unit;
	sc->sc_iot = tipb->tipb_iot;
	sc->sc_intr = tipb->tipb_intr;
	sc->sc_addr = tipb->tipb_addr;
	sc->sc_bot = 1; /* use the bottom timer in all cases */
	sc->sc_size = OMAPL1X_TIMER_SIZE;

	if (bus_space_map(sc->sc_iot, sc->sc_addr, sc->sc_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	aprint_normal("\n");
	aprint_naive("\n");

	switch (sc->sc_timerno) {
	case 0:
		/*
		 * timer #0 is the system clock
		 * it gets started later
		 */
		clock_sc = sc;
		sc->sc_timer_freq = OMAPL1X_TIMER0_FREQ;
		break;
	case 1:
		/*
		 * timer #2 is the stat clock
		 * it gets started later
		 */
		profhz = stathz = STATHZ;
		stat_sc = sc;
		sc->sc_timer_freq = OMAPL1X_TIMER2_FREQ;
		break;
	case 2:
		/*
		 * Timer #3 is used for microtime reference clock and for delay()
		 * autoloading, non-interrupting, just wraps around as an unsigned int.
		 * we start it now to make delay() available
		 */
		ref_sc = sc;
		sc->sc_timer_freq = OMAPL1X_TIMER3_FREQ;
		omapl1x_microtime_init();
		break;
	default:
		panic("bad omapl1x timer number %d\n", sc->sc_timerno);
		break;
	}

	wdt.wdt_iot = tipb->tipb_iot;
	wdt.wdt_addr = OMAPL1X_WDT_ADDR;
	wdt.wdt_size = OMAPL1X_WDT_SIZE;

	/* Map WDT registers. We want to use it for reseting the chip */
	if (bus_space_map(wdt.wdt_iot, wdt.wdt_addr,
			  wdt.wdt_size, 0, &wdt.wdt_ioh)) {
		aprint_error_dev(self, "can't map wdt mem space\n");
		return;
	}
}

void
omapl1x_reset (void)
{
	uint32_t val;

	printf("\n");
	delay(50000);

	val = bus_space_read_4(wdt.wdt_iot, wdt.wdt_ioh, TGCR);

	/*
	 * Get the timer out of reset  and put it in
	 * watchdog timer mode.
	 */
	val |= ((TGCR_RS_RUN << TGCR_RS_12_SHIFT) |
		(TGCR_RS_RUN << TGCR_RS_34_SHIFT));
	val |= (TGCR_TIMMODE_64BIT_WDOG << TGCR_TIMMODE_SHIFT);

	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, TGCR, val);

	/* Init the counter and period registers */
	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, TIM12, 0x0);
	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, TIM34, 0x0);
	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, PRD12, ~0);
	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, PRD34, ~0);

	val = bus_space_read_4(wdt.wdt_iot, wdt.wdt_ioh, WDTCR);

	/*
	 * Now enable the wdt and write the WDKEY1 to get the 
	 * wd in the Pre-active state.
	 */
	val |= (1 << WDTCR_WDEN_SHIFT);
	val |= (WDTCR_WDTKEY1 << WDTCR_WDKEY_SHIFT);

	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, WDTCR, val);

	/*
	 * Now write the WDKEY2 to get the wd in the Active 
	 * state.
	 */
	val = bus_space_read_4(wdt.wdt_iot, wdt.wdt_ioh, WDTCR);
	val &= ~WDTCR_WDKEY_MASK;
	val |= (WDTCR_WDTKEY2 << WDTCR_WDKEY_SHIFT);
	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, WDTCR, val);

	/* 
	 * Write an invalid value to the WDKEY to trigger
	 * the wd timeout right away.
	 */
	val = bus_space_read_4(wdt.wdt_iot, wdt.wdt_ioh, WDTCR);
	val &= ~WDTCR_WDKEY_MASK;
	bus_space_write_4(wdt.wdt_iot, wdt.wdt_ioh, WDTCR, val);

	while(1);
}
