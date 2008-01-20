/*	$NetBSD: sa11x0_ost.c,v 1.20.44.1 2008/01/20 17:51:06 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa11x0_ost.c,v 1.20.44.1 2008/01/20 17:51:06 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/sa11x0/sa11x0_reg.h> 
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_ostreg.h>

static int	saost_match(struct device *, struct cfdata *, void *);
static void	saost_attach(struct device *, struct device *, void *);

static void	saost_tc_init(void);

static uint32_t	gettick(void);
static int	clockintr(void *);
static int	statintr(void *);

struct saost_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	uint32_t		sc_clock_count;
	uint32_t		sc_statclock_count;
	uint32_t		sc_statclock_step;
};

static struct saost_softc *saost_sc = NULL;

#define TIMER_FREQUENCY         3686400         /* 3.6864MHz */

#ifndef STATHZ
#define STATHZ	64
#endif

CFATTACH_DECL(saost, sizeof(struct saost_softc),
    saost_match, saost_attach, NULL, NULL);

static int
saost_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

static void
saost_attach(struct device *parent, struct device *self, void *aux)
{
	struct saost_softc *sc = (struct saost_softc *)self;
	struct sa11x0_attach_args *sa = aux;

	printf("\n");

	sc->sc_iot = sa->sa_iot;

	saost_sc = sc;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, 
	    &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	/* disable all channel and clear interrupt status */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_IR, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_SR, 0xf);

	printf("%s: SA-11x0 OS Timer\n", sc->sc_dev.dv_xname);
}

static int
clockintr(void *arg)
{
	struct saost_softc *sc = saost_sc;
	struct clockframe *frame = arg;
	uint32_t oscr, nextmatch, oldmatch;
	int s;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_SR, 1);

	/* schedule next clock intr */
	oldmatch = sc->sc_clock_count;
	nextmatch = oldmatch + TIMER_FREQUENCY / hz;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR0, nextmatch);
	oscr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SAOST_CR);

	if ((nextmatch > oldmatch &&
	     (oscr > nextmatch || oscr < oldmatch)) ||
	    (nextmatch < oldmatch && oscr > nextmatch && oscr < oldmatch)) {
		/*
		 * we couldn't set the matching register in time.
		 * just set it to some value so that next interrupt happens.
		 * XXX is it possible to compensate lost interrupts?
		 */

		s = splhigh();
		oscr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SAOST_CR);
		nextmatch = oscr + 10;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR0, nextmatch);
		splx(s);
	}

	sc->sc_clock_count = nextmatch;
	hardclock(frame);

	return 1;
}

static int
statintr(void *arg)
{
	struct saost_softc *sc = saost_sc;
	struct clockframe *frame = arg;
	uint32_t oscr, nextmatch, oldmatch;
	int s;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_SR, 2);

	/* schedule next clock intr */
	oldmatch = sc->sc_statclock_count;
	nextmatch = oldmatch + sc->sc_statclock_step;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR1, nextmatch);
	oscr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SAOST_CR);

	if ((nextmatch > oldmatch &&
	     (oscr > nextmatch || oscr < oldmatch)) ||
	    (nextmatch < oldmatch && oscr > nextmatch && oscr < oldmatch)) {
		/*
		 * we couldn't set the matching register in time.
		 * just set it to some value so that next interrupt happens.
		 * XXX is it possible to compensate lost interrupts?
		 */

		s = splhigh();
		oscr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SAOST_CR);
		nextmatch = oscr + 10;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR1, nextmatch);
		splx(s);
	}

	sc->sc_statclock_count = nextmatch;
	statclock(frame);

	return 1;
}

void
setstatclockrate(int schz)
{
	struct saost_softc *sc = saost_sc;
	uint32_t count;

	sc->sc_statclock_step = TIMER_FREQUENCY / schz;
	count = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SAOST_CR);
	count += sc->sc_statclock_step;
	sc->sc_statclock_count = count;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR1, count);
}

void
cpu_initclocks(void)
{
	struct saost_softc *sc = saost_sc;

	stathz = STATHZ;
	profhz = stathz;
	sc->sc_statclock_step = TIMER_FREQUENCY / stathz;

	printf("clock: hz=%d stathz=%d\n", hz, stathz);

	/* Use the channels 0 and 1 for hardclock and statclock, respectively */
	sc->sc_clock_count = TIMER_FREQUENCY / hz;
	sc->sc_statclock_count = TIMER_FREQUENCY / stathz;

	sa11x0_intr_establish(0, 26, 1, IPL_CLOCK, clockintr, 0);
	sa11x0_intr_establish(0, 27, 1, IPL_CLOCK, statintr, 0);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_SR, 0xf);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_IR, 3);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR0,
			  sc->sc_clock_count);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_MR1,
			  sc->sc_statclock_count);

	/* Zero the counter value */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAOST_CR, 0);

	saost_tc_init();
}

static u_int
saost_tc_get_timecount(struct timecounter *tc)
{
	return (u_int)gettick();
}

static void
saost_tc_init(void)
{
	static struct timecounter saost_tc = {
		.tc_get_timecount = saost_tc_get_timecount,
		.tc_frequency = TIMER_FREQUENCY,
		.tc_counter_mask = ~0,
		.tc_name = "saost_count",
		.tc_quality = 100,
	};

	tc_init(&saost_tc);
}

static uint32_t
gettick(void)
{
	struct saost_softc *sc = saost_sc;
	uint32_t counter;
	u_int saved_ints;

	saved_ints = disable_interrupts(I32_bit);
	counter = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SAOST_CR);
	restore_interrupts(saved_ints);

	return counter;
}

void
delay(u_int usecs)
{
	uint32_t xtick, otick, delta;
	int j, csec, usec;

	csec = usecs / 10000;
	usec = usecs % 10000;
	
	usecs = (TIMER_FREQUENCY / 100) * csec
	    + (TIMER_FREQUENCY / 100) * usec / 10000;

	if (saost_sc == NULL) {
		/* clock isn't initialized yet */
		for (; usecs > 0; usecs--)
			for (j = 100; j > 0; j--)
				continue;
		return;
	}

	otick = gettick();

	while (1) {
		for (j = 100; j > 0; j--)
			continue;
		xtick = gettick();
		delta = xtick - otick;
		if (delta > usecs)
			break;
		usecs -= delta;
		otick = xtick;
	}
}

#ifndef __HAVE_GENERIC_TODR
void
resettodr(void)
{
}

void
inittodr(time_t base)
{
	time.tv_sec = base;
	time.tv_usec = 0;
}
#endif /* !__HAVE_GENERIC_TODR */
