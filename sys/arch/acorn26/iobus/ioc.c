/* $NetBSD: ioc.c,v 1.15.28.1 2008/01/09 01:44:27 matt Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * ioc.c - Acorn/ARM I/O Controller (Albion/VC2311/VL2311/VY86C410)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ioc.c,v 1.15.28.1 2008/01/09 01:44:27 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/reboot.h>	/* For bootverbose */
#include <sys/systm.h>
#include <sys/timetc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/irq.h>

#include <arch/acorn26/acorn26/cpuvar.h>
#include <arch/acorn26/iobus/iobusvar.h>
#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/iobus/iocreg.h>

#include "locators.h"

static int ioc_match(struct device *parent, struct cfdata *cf, void *aux);
static void ioc_attach(struct device *parent, struct device *self, void *aux);
static int ioc_search(struct device *parent, struct cfdata *cf,
		      const int *ldesc, void *aux);
static int ioc_print(void *aux, const char *pnp);
static int ioc_irq_clock(void *cookie);
static int ioc_irq_statclock(void *cookie);
static u_int ioc_get_timecount(struct timecounter *);

CFATTACH_DECL(ioc, sizeof(struct ioc_softc),
    ioc_match, ioc_attach, NULL, NULL);

struct device *the_ioc;

/*
 * Autoconfiguration glue
 */

static int
ioc_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/*
	 * This is tricky.  Accessing non-existent devices in iobus
	 * space can hang the machine (MEMC datasheet section 5.3.3),
	 * so probes would have to be very delicate.  This isn't
	 * _much_ of a problem with the IOC, since all machines I know
	 * of have exactly one.
	 */
	if (the_ioc == NULL)
		return 1;
	return 0;
}

static void
ioc_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioc_softc *sc = (void *)self;
	struct iobus_attach_args *ioa = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	the_ioc = self;
	sc->sc_bst = ioa->ioa_tag;
	if (bus_space_map(ioa->ioa_tag, ioa->ioa_base, 0x00200000,
			  0, &(sc->sc_bsh)) != 0)
		panic("%s: couldn't map", sc->sc_dev.dv_xname);
	bst = sc->sc_bst;
	bsh = sc->sc_bsh;
	/* Now we need to set up bits of the IOC */
	/* Control register: All bits high (input) is probably safe */
	ioc_ctl_write(self, 0xff, 0xff);
	/*
	 * IRQ/FIQ: mask out all, leave clearing latched interrupts
	 * till someone asks.
	 */
	ioc_irq_setmask(0);
	ioc_fiq_setmask(0);
	/*-
	 * Timers:
	 * Timers 0/1 are set up by ioc_initclocks (called by cpu_initclocks).
	 * XXX What if we need timers before then?
	 * Timer 2 is set up by whatever's connected to BAUD.
	 * Timer 3 is set up by the arckbd driver.
	 */
	printf("\n");

	config_search_ia(ioc_search, self, "ioc", NULL);
}

extern struct bus_space ioc_bs_tag;

static int
ioc_search(struct device *parent, struct cfdata *cf,
	   const int *ldesc, void *aux)
{
	struct ioc_softc *sc = (void *)parent;
	struct ioc_attach_args ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	
	ioc.ioc_bank = cf->cf_loc[IOCCF_BANK];
	ioc.ioc_offset = cf->cf_loc[IOCCF_OFFSET];
	ioc.ioc_slow_t = bst;
	bus_space_subregion(bst, bsh, (ioc.ioc_bank << IOC_BANK_SHIFT)
			    + (IOC_TYPE_SLOW << IOC_TYPE_SHIFT)
			    + (ioc.ioc_offset >> 2),
			    1 << IOC_BANK_SHIFT, &ioc.ioc_slow_h);
	ioc.ioc_medium_t = bst;
	bus_space_subregion(bst, bsh, (ioc.ioc_bank << IOC_BANK_SHIFT)
			    + (IOC_TYPE_MEDIUM << IOC_TYPE_SHIFT)
			    + (ioc.ioc_offset >> 2),
			    1 << IOC_BANK_SHIFT, &ioc.ioc_medium_h);
	ioc.ioc_fast_t = bst;
	bus_space_subregion(bst, bsh, (ioc.ioc_bank << IOC_BANK_SHIFT)
			    + (IOC_TYPE_FAST << IOC_TYPE_SHIFT)
			    + (ioc.ioc_offset >> 2),
			    1 << IOC_BANK_SHIFT, &ioc.ioc_fast_h);
	ioc.ioc_sync_t = bst;
	bus_space_subregion(bst, bsh, (ioc.ioc_bank << IOC_BANK_SHIFT)
			    + (IOC_TYPE_SYNC << IOC_TYPE_SHIFT)
			    + (ioc.ioc_offset >> 2),
			    1 << IOC_BANK_SHIFT, &ioc.ioc_sync_h);
	if (config_match(parent, cf, &ioc) > 0)
		config_attach(parent, cf, &ioc, ioc_print);

	return 0;
}

static int
ioc_print(void *aux, const char *pnp)
{
	struct ioc_attach_args *ioc = aux;

	if (ioc->ioc_bank != IOCCF_BANK_DEFAULT)
		aprint_normal(" bank %d", ioc->ioc_bank);
	if (ioc->ioc_offset != IOCCF_OFFSET_DEFAULT)
		aprint_normal(" offset 0x%02x", ioc->ioc_offset);
	return UNCONF;
}

/*
 * Find out if an interrupt line is currently active
 */

int
ioc_irq_status(int irq)
{
	struct ioc_softc *sc = (void *)the_ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	if (irq < 8)
		return (bus_space_read_1(bst, bsh, IOC_IRQSTA) &
			IOC_IRQA_BIT(irq)) != 0;
	else
		return (bus_space_read_1(bst, bsh, IOC_IRQSTB) &
			IOC_IRQB_BIT(irq)) != 0;
}

u_int32_t
ioc_irq_status_full()
{
	struct ioc_softc *sc = (void *)the_ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

#if 0 /* XXX */
	printf("IRQ mask: 0x%x\n",
	       bus_space_read_1(bst, bsh, IOC_IRQMSKA) | 
	       (bus_space_read_1(bst, bsh, IOC_IRQMSKB) << 8));
#endif
	return bus_space_read_1(bst, bsh, IOC_IRQRQA) |
	    (bus_space_read_1(bst, bsh, IOC_IRQRQB) << 8);
}

void
ioc_irq_setmask(u_int32_t mask)
{
	struct ioc_softc *sc = (void *)the_ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	bus_space_write_1(bst, bsh, IOC_IRQMSKA, mask & 0xff);
	bus_space_write_1(bst, bsh, IOC_IRQMSKB, (mask >> 8) & 0xff);
}

void
ioc_irq_waitfor(int irq)
{

	while (!ioc_irq_status(irq));
}

void
ioc_irq_clear(int mask)
{
	struct ioc_softc *sc = (void *)the_ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	bus_space_write_1(bst, bsh, IOC_IRQRQA, mask);
}

#if 0

/*
 * ioc_get_irq_level:
 *
 * Find out the current level of an edge-triggered interrupt line.
 * Useful for the VIDC driver to know if it's in VSYNC if nothing
 * else.
 */

int ioc_get_irq_level(struct device *self, int irq)
{
	struct ioc_softc *sc = (void *)self;

	switch (irq) {
	case IOC_IRQ_IF:
		return (bus_space_read_1(sc->sc_bst, sc->sc_bsh, IOC_CTL) &
			IOC_CTL_NIF) != 0;
	case IOC_IRQ_IR:
		return (bus_space_read_1(sc->sc_bst, sc->sc_bsh, IOC_CTL) &
			IOC_CTL_IR) != 0;
	}
	panic("ioc_get_irq_level called for irq %d, which isn't edge-triggered",
	      irq);
}

#endif /* 0 */

/*
 * FIQs
 */

void
ioc_fiq_setmask(u_int32_t mask)
{
	struct ioc_softc *sc = (void *)the_ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	bus_space_write_1(bst, bsh, IOC_FIQMSK, mask);
}



/*
 * Counters
 */

void ioc_counter_start(struct device *self, int counter, int value)
{
	struct ioc_softc *sc = (void *)self;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int tlow, thigh, tgo;

	switch (counter) {
	case 0:	tlow = IOC_T0LOW; thigh = IOC_T0HIGH; tgo = IOC_T0GO; break;
	case 1:	tlow = IOC_T1LOW; thigh = IOC_T1HIGH; tgo = IOC_T1GO; break;
	case 2:	tlow = IOC_T2LOW; thigh = IOC_T2HIGH; tgo = IOC_T2GO; break;
	case 3:	tlow = IOC_T3LOW; thigh = IOC_T3HIGH; tgo = IOC_T3GO; break;
	default: panic("%s: ioc_counter_start: bad counter (%d)",
		       self->dv_xname, counter);
	}
	bus_space_write_1(bst, bsh, tlow, value & 0xff);
	bus_space_write_1(bst, bsh, thigh, value >> 8 & 0xff);
	bus_space_write_1(bst, bsh, tgo, 0);
}

/* Cache to save microtime recalculating it */
static int t0_count;
/*
 * Statistics clock interval and variance, in ticks.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
int statvar = 8192;
int statmin;
	
void
cpu_initclocks(void)
{
	struct ioc_softc *sc;
	int minint, statint;

	KASSERT(the_ioc != NULL);
	sc = (struct ioc_softc *)the_ioc;
	stathz = hz; /* XXX what _should_ it be? */

	if (hz == 0 || IOC_TIMER_RATE % hz != 0 ||
	    (t0_count = IOC_TIMER_RATE / hz - 1) > 65535)
		panic("ioc_initclocks: Impossible clock rate: %d Hz", hz);
	ioc_counter_start(the_ioc, 0, t0_count);
	evcnt_attach_dynamic(&sc->sc_clkev, EVCNT_TYPE_INTR, NULL,
	    sc->sc_dev.dv_xname, "clock");
	sc->sc_clkirq = irq_establish(IOC_IRQ_TM0, IPL_CLOCK, ioc_irq_clock,
	    NULL, &sc->sc_clkev);
	sc->sc_tc.tc_get_timecount = ioc_get_timecount;
	sc->sc_tc.tc_counter_mask = ~(u_int)0;
	sc->sc_tc.tc_frequency = IOC_TIMER_RATE;
	sc->sc_tc.tc_name = sc->sc_dev.dv_xname;
	sc->sc_tc.tc_quality = 100;
	sc->sc_tc.tc_priv = sc;
	tc_init(&sc->sc_tc);
	if (bootverbose)
		printf("%s: %d Hz clock interrupting at %s\n",
		    the_ioc->dv_xname, hz, irq_string(sc->sc_clkirq));
	
	if (stathz) {
		profhz = stathz; /* Makes life simpler */
		
		if (stathz == 0 || IOC_TIMER_RATE % stathz != 0 ||
		    (statint = IOC_TIMER_RATE / stathz - 1) > 65535)
			panic("Impossible statclock rate: %d Hz", stathz);

		minint = statint / 2 + 100;
		while (statvar > minint)
			statvar >>= 1;
		statmin = statint - (statvar >> 1);

		ioc_counter_start(the_ioc, 1, statint);

		evcnt_attach_dynamic(&sc->sc_sclkev, EVCNT_TYPE_INTR, NULL,
		    sc->sc_dev.dv_xname, "statclock");
		sc->sc_sclkirq = irq_establish(IOC_IRQ_TM1, IPL_HIGH,
		    ioc_irq_statclock, NULL, &sc->sc_sclkev);
		if (bootverbose)
			printf("%s: %d Hz statclock interrupting at %s\n",
			    the_ioc->dv_xname, stathz,
			    irq_string(sc->sc_sclkirq));
	}
}

static int
ioc_irq_clock(void *cookie)
{
	struct ioc_softc *sc = (void *)the_ioc;

	sc->sc_tcbase += t0_count + 1;
	hardclock(cookie);
	return IRQ_HANDLED;
}

static int
ioc_irq_statclock(void *cookie)
{
	struct ioc_softc *sc = (void *)the_ioc;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int r, newint;

	statclock(cookie);

	/* Generate a new randomly-distributed clock period. */
	do {
		r = random() & (statvar - 1);
	} while (r == 0);
	newint = statmin + r;

	/*
	 * Load the next clock period into the latch, but don't do anything
	 * with it.  It'll be used for the _next_ statclock reload.
	 */
	bus_space_write_1(bst, bsh, IOC_T1LOW, newint & 0xff);
	bus_space_write_1(bst, bsh, IOC_T1HIGH, newint >> 8 & 0xff);
	return IRQ_HANDLED;
}

void
setstatclockrate(int hzrate)
{

	/* Nothing to do here -- we've forced stathz == profhz above. */
	KASSERT(hzrate == stathz);
}

/*
 * IOC timecounter
 *
 * We construct a timecounter from timer 0, which is also running the
 * hardclock interrupt.  Since the timer 0 resets on every hardclock
 * interrupt, we keep track of the high-order bits of the counter in
 * software, incrementing it on every hardclock.  If hardclock
 * interrupts are disabled, there's a period where the timer has reset
 * but the interrupt handler hasn't incremented the hight-order bits.
 * We detect this by checking whether there's a hardclock interrupt
 * pending.  We take a bit of extra care to ensure that we aren't
 * confused by the interrupt happening between our latching the
 * timer's count and reading the interrupt flag.
 */
static u_int
ioc_get_timecount(struct timecounter *tc)
{
	struct ioc_softc *sc = tc->tc_priv;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	u_int t0, count;
	int s, intpending;

	s = splclock();
	bus_space_write_1(bst, bsh, IOC_T0LATCH, 0);
	if (__predict_false((intpending = ioc_irq_status(IOC_IRQ_TM0))))
		bus_space_write_1(bst, bsh, IOC_T0LATCH, 0);	
	t0 = bus_space_read_1(bst, bsh, IOC_T0LOW);
	t0 += bus_space_read_1(bst, bsh, IOC_T0HIGH) << 8;
	count = sc->sc_tcbase - t0;
	if (intpending)
		count += t0_count + 1;
	splx(s);
	return count;
}

void
delay(u_int usecs)
{

	if (usecs <= 10 || cold)
		cpu_delayloop(usecs * cpu_delay_factor);
	else {
		struct timeval start, gap, now, end;

		microtime(&start);
		gap.tv_sec = usecs / 1000000;
		gap.tv_usec = usecs % 1000000;
		timeradd(&start, &gap, &end);
		do {
			microtime(&now);
		} while (timercmp(&now, &end, <));
	}

}
