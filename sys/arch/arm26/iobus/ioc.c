/* $NetBSD: ioc.c,v 1.6 2001/01/07 15:36:34 bjh21 Exp $ */

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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * ioc.c - Acorn/ARM I/O Controller (Albion/VC2311/VL2311/VY86C410)
 */

#include <sys/param.h>

__RCSID("$NetBSD: ioc.c,v 1.6 2001/01/07 15:36:34 bjh21 Exp $");

#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/reboot.h>	/* For bootverbose */
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/irq.h>

#include <arch/arm26/arm26/cpuvar.h>
#include <arch/arm26/iobus/iobusvar.h>
#include <arch/arm26/iobus/iocvar.h>
#include <arch/arm26/iobus/iocreg.h>

#include "locators.h"

static int ioc_match(struct device *parent, struct cfdata *cf, void *aux);
static void ioc_attach(struct device *parent, struct device *self, void *aux);
static int ioc_search(struct device *parent, struct cfdata *cf, void *aux);
static int ioc_print(void *aux, const char *pnp);
static int ioc_irq_clock(void *cookie);
static int ioc_irq_statclock(void *cookie);

struct ioc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct irq_handler	*sc_clkirq;
	struct irq_handler	*sc_sclkirq;
	u_int8_t		sc_ctl;
};

struct cfattach ioc_ca = {
	sizeof(struct ioc_softc), ioc_match, ioc_attach
};

extern struct cfdriver ioc_cd;

/*
 * Autoconfiguration glue
 */

static int
ioc_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/*
	 * This is tricky.  Accessing non-existant devices in iobus
	 * space can hang the machine (MEMC datasheet section 5.3.3),
	 * so probes would have to be very delicate.  This isn't
	 * _much_ of a problem with the IOC, since all machines I know
	 * of have exactly one.
	 */
	if (cf->cf_unit == 0)
		return 1;
	else
		return 0;
}

static void
ioc_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioc_softc *sc = (void *)self;
	struct iobus_attach_args *ioa = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

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
	 *
	 * In fact, the masks will be in this state already.  See
	 * start.c for details.
	 */
	bus_space_write_1(bst, bsh, IOC_IRQMSKA, 0x00);
	bus_space_write_1(bst, bsh, IOC_IRQMSKB, 0x00);
	bus_space_write_1(bst, bsh, IOC_FIQMSK,  0x00);
	/*-
	 * Timers:
	 * Timers 0/1 are set up by ioc_initclocks (called by cpu_initclocks).
	 * XXX What if we need timers before then?
	 * Timer 2 is set up by whatever's connected to BAUD.
	 * Timer 3 is set up by the arckbd driver.
	 */
	printf("\n");

	config_search(ioc_search, self, NULL);
}

extern struct bus_space ioc_bs_tag;

static int
ioc_search(struct device *parent, struct cfdata *cf, void *aux)
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
	if ((cf->cf_attach->ca_match)(parent, cf, &ioc) > 0)
		config_attach(parent, cf, &ioc, ioc_print);

	return 0;
}

static int
ioc_print(void *aux, const char *pnp)
{
	struct ioc_attach_args *ioc = aux;

	if (ioc->ioc_bank != IOCCF_BANK_DEFAULT)
		printf(" bank %d", ioc->ioc_bank);
	if (ioc->ioc_offset != IOCCF_OFFSET_DEFAULT)
		printf(" offset 0x%02x", ioc->ioc_offset);
	return UNCONF;
}

/*
 * Control Register
 */

/*
 * ioc_ctl_{read,write}
 *
 * Functions to manipulate the IOC control register.  The bottom six
 * bits of the control register map to bidirectional pins on the chip.
 * The output circuits are open-drain, so a pin is made an input by
 * writing '1' to it.
 */

u_int
ioc_ctl_read(struct device *self)
{
	struct ioc_softc *sc = (void *)self;

	return bus_space_read_1(sc->sc_bst, sc->sc_bsh, IOC_CTL);
}

void
ioc_ctl_write(struct device *self, u_int value, u_int mask)
{
	struct ioc_softc *sc = (void *)self;
	int s;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	
	s = splhigh();
	sc->sc_ctl = (sc->sc_ctl & ~mask) | (value & mask);
	bus_space_barrier(bst, bsh, IOC_CTL, 1, BUS_BARRIER_WRITE);
	bus_space_write_1(bst, bsh, IOC_CTL, sc->sc_ctl);
	bus_space_barrier(bst, bsh, IOC_CTL, 1, BUS_BARRIER_WRITE);
	splx(s);
}

/*
 * Find out if an interrupt line is currently active
 */

int
ioc_irq_status(struct device *self, int irq)
{
	struct ioc_softc *sc = (void *)self;
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
ioc_irq_status_full(struct device *self)
{
	struct ioc_softc *sc = (void *)self;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

#if 0 /* XXX */
	printf("IRQ mask: 0x%x\n",
	       bus_space_read_1(bst, bsh, IOC_IRQMSKA) | 
	       (bus_space_read_1(bst, bsh, IOC_IRQMSKB) << 8));
#endif
	return bus_space_read_1(bst, bsh, IOC_IRQSTA) |
	    (bus_space_read_1(bst, bsh, IOC_IRQSTB) << 8);
}

void
ioc_irq_setmask(struct device *self, u_int32_t mask)
{
	struct ioc_softc *sc = (void *)self;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	bus_space_write_1(bst, bsh, IOC_IRQMSKA, mask & 0xff);
	bus_space_write_1(bst, bsh, IOC_IRQMSKB, (mask >> 8) & 0xff);
}

void
ioc_irq_waitfor(struct device *self, int irq)
{

	while (!ioc_irq_status(self, irq));
}

void
ioc_irq_clear(struct device *self, int mask)
{
	struct ioc_softc *sc = (void *)self;
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
	bus_space_barrier(bst, bsh, tlow, tgo - tlow + 1, BUS_BARRIER_WRITE);
	bus_space_write_1(bst, bsh, tlow, value & 0xff);
	bus_space_write_1(bst, bsh, thigh, value >> 8 & 0xff);
	bus_space_barrier(bst, bsh, tlow, tgo - tlow + 1, BUS_BARRIER_WRITE);
	bus_space_write_1(bst, bsh, tgo, 0);
	bus_space_barrier(bst, bsh, tlow, tgo - tlow, BUS_BARRIER_WRITE);
}

/* Cache to save microtime recalculating it */
static int t0_count;
	
void
cpu_initclocks(void)
{
	struct device *self;
	struct ioc_softc *sc;

#ifdef DIAGNOSTIC
	if (ioc_cd.cd_ndevs <= 0 || ioc_cd.cd_devs[0] == NULL)
		panic("cpu_initclocks: no ioc0");
#endif
	self = ioc_cd.cd_devs[0];
	sc = (struct ioc_softc *)self;
	stathz = hz; /* XXX what _should_ it be? */

	if (hz == 0 || IOC_TIMER_RATE % hz != 0 ||
	    (t0_count = IOC_TIMER_RATE / hz) > 65535)
		panic("ioc_initclocks: Impossible clock rate: %d Hz", hz);
	ioc_counter_start(self, 0, t0_count);
	sc->sc_clkirq = irq_establish(IOC_IRQ_TM0, IPL_CLOCK, ioc_irq_clock,
	    NULL);
	if (bootverbose)
		printf("%s: %d Hz clock interrupting at %s\n",
		    self->dv_xname, hz, irq_string(sc->sc_clkirq));
	
	if (stathz) {
		setstatclockrate(stathz);
		sc->sc_sclkirq = irq_establish(IOC_IRQ_TM1, IPL_STATCLOCK,
						   ioc_irq_statclock, NULL);
		if (bootverbose)
			printf("%s: %d Hz statclock interrupting at %s\n",
			    self->dv_xname, stathz, irq_string(sc->sc_sclkirq));
	}
}

static int
ioc_irq_clock(void *cookie)
{

	hardclock(cookie);
	return IRQ_HANDLED;
}

static int
ioc_irq_statclock(void *cookie)
{

	statclock(cookie);
	return IRQ_HANDLED;
}

void
setstatclockrate(int hzrate)
{
	struct device *self;
	int count;

#ifdef DIAGNOSTIC
	if (ioc_cd.cd_ndevs <= 0 || ioc_cd.cd_devs[0] == NULL)
		panic("setstatclockrate: no ioc0");
#endif
	self = ioc_cd.cd_devs[0];

	/* XXX This currently restarts the counter -- should it? */
	if (hzrate == 0 || IOC_TIMER_RATE % hzrate != 0 ||
	    (count = IOC_TIMER_RATE / hz) > 65535)
		panic("Impossible statclock rate: %d Hz", hzrate);
	ioc_counter_start(self, 1, count);
}

void
microtime(struct timeval *tv)
{
	struct device *self;
	struct ioc_softc *sc;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	int t0, s, intbefore, intafter;

#ifdef DIAGNOSTIC
	if (ioc_cd.cd_ndevs <= 0 || ioc_cd.cd_devs[0] == NULL)
		panic("microtime: no ioc0");
#endif
	self = ioc_cd.cd_devs[0];
	sc = (struct ioc_softc *)self;

	bst = sc->sc_bst;
	bsh = sc->sc_bsh;

	s = splclock();

	*tv = time;

	intbefore = ioc_irq_status(self, IOC_IRQ_TM0);
	bus_space_write_1(bst, bsh, IOC_T0LATCH, 0);
	t0 = bus_space_read_1(bst, bsh, IOC_T0LOW);
	t0 += bus_space_read_1(bst, bsh, IOC_T0HIGH) << 8;
	intafter = ioc_irq_status(self, IOC_IRQ_TM0);

	splx(s);

	/*
	 * If there's a timer interrupt pending, the counter has
	 * probably wrapped around once since "time" was last updated.
	 * Things are complicated by the fact that this could happen
	 * while we're trying to work out the time.  We include some
	 * heuristics to spot this.
	 */
	
	if (intbefore || (intafter && t0 < t0_count / 2))
		t0 -= t0_count;

	tv->tv_usec += (t0_count - t0) / (IOC_TIMER_RATE / 1000000);
	
	while (tv->tv_usec > 1000000) {
		tv->tv_sec += 1;
		tv->tv_usec -= 1000000;
	}
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
