/*	$NetBSD: ixp12x0_clk.c,v 1.9.2.2 2008/01/21 09:35:44 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ixp12x0_clk.c,v 1.9.2.2 2008/01/21 09:35:44 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ixp12x0/ixpsipvar.h> 

#include <arm/ixp12x0/ixp12x0_pcireg.h> 
#include <arm/ixp12x0/ixp12x0_clkreg.h> 
#include <arm/ixp12x0/ixp12x0var.h>

static int	ixpclk_match(struct device *, struct cfdata *, void *);
static void	ixpclk_attach(struct device *, struct device *, void *);

static u_int	ixpclk_get_timecount(struct timecounter *);

int		gettick(void);
void		rtcinit(void);

/* callback functions for intr_functions */
static int      ixpclk_intr(void* arg);

struct ixpclk_softc {
	struct device		sc_dev;
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_pll_ioh;

	u_int32_t		sc_clock_count;
	u_int32_t		sc_count_per_usec;
	u_int32_t		sc_coreclock_freq;
};

#define XTAL_FREQ		3686400		/* 3.6864MHz */
#define XTAL_FREQ3686400
#undef XTAL_FREQ3787800
#undef XTAL_FREQ3579500
#define	MAX_CCF			22

#if defined(XTAL_FREQ3686400)
static u_int32_t ccf_to_coreclock[MAX_CCF + 1] = {
	29491000,
	36865000,
	44237000,
	51610000,
	58982000,
	66355000,
	73728000,
	81101000,
	88474000,
	95846000,
	103219000,
	110592000,
	132710000,
	147456000,
	154829000,
	162202000,
	165890000,
	176947000,
	191693000,
	199066000,
	206438000,
	221184000,
	232243000,
};
#elif defined(XTAL_FREQ3787800)
#elif defined(XTAL_FREQ3579500)
#else
#error
#endif

static struct ixpclk_softc *ixpclk_sc = NULL;

static struct timecounter ixpclk_timecounter = {
	ixpclk_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	0,			/* frequency */
	"ixpclk",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static volatile uint32_t ixpclk_base;

#define TIMER_FREQUENCY         3686400         /* 3.6864MHz */
#define TICKS_PER_MICROSECOND   (TIMER_FREQUENCY/1000000)

CFATTACH_DECL(ixpclk, sizeof(struct ixpclk_softc),
    ixpclk_match, ixpclk_attach, NULL, NULL);

#define GET_TIMER_VALUE(sc)	(bus_space_read_4((sc)->sc_iot,		\
						  (sc)->sc_ioh,		\
						  IXPCLK_VALUE)		\
				 & IXPCL_CTV)

static int
ixpclk_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 2;
}

static void
ixpclk_attach(struct device *parent, struct device *self, void *aux)
{
	struct ixpclk_softc		*sc;
	struct ixpsip_attach_args	*sa;
	u_int32_t			ccf;
	bool first_run = ixpclk_sc == NULL;

	printf("\n");

	sc = (struct ixpclk_softc*) self;
	sa = aux;
	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;

	/* using first timer for system ticks */
	if (ixpclk_sc == NULL)
		ixpclk_sc = sc;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);
	if (bus_space_map(sa->sa_iot, sa->sa_addr + IXPCLK_PLL_CFG_OFFSET,
			  IXPCLK_PLL_CFG_SIZE, 0, &sc->sc_pll_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	/* disable all channel and clear interrupt status */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CONTROL,
			  IXPCL_DISABLE | IXPCL_PERIODIC | IXPCL_STP_CORE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CLEAR, 0);


	ccf = bus_space_read_4(sc->sc_iot, sc->sc_pll_ioh, 0)
		& IXP12X0_PLL_CFG_CCF;
	sc->sc_coreclock_freq = ccf_to_coreclock[ccf];

	sc->sc_clock_count = sc->sc_coreclock_freq / hz;
	sc->sc_count_per_usec = sc->sc_coreclock_freq / 1000000;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CLEAR, IXPT_CLEAR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_LOAD,
			  sc->sc_clock_count);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CONTROL,
			  IXPCL_ENABLE | IXPCL_PERIODIC | IXPCL_STP_CORE);

	if (first_run) {
		ixpclk_timecounter.tc_frequency = sc->sc_coreclock_freq;
		tc_init(&ixpclk_timecounter);
	}

	printf("%s: IXP12x0 Interval Timer (core clock %d.%03dMHz)\n",
	       sc->sc_dev.dv_xname,
	       sc->sc_coreclock_freq / 1000000,
	       (sc->sc_coreclock_freq % 1000000) / 1000);
}

/*
 * ixpclk_intr:
 *
 *	Handle the hardclock interrupt.
 */
static int
ixpclk_intr(void *arg)
{

	bus_space_write_4(ixpclk_sc->sc_iot, ixpclk_sc->sc_ioh,
			  IXPCLK_CLEAR, 1);

	atomic_add_32(&ixpclk_base, ixpclk_sc->sc_coreclock_freq);

	hardclock((struct clockframe*) arg);
	return (1);
}

/*
 * setstatclockrate:
 *
 *	Set the rate of the statistics clock.
 *
 *	We assume that hz is either stathz or profhz, and that neither
 *	will change after being set by cpu_initclocks().  We could
 *	recalculate the intervals here, but that would be a pain.
 */
void
setstatclockrate(int newhz)
{

	/* use hardclock */

	/* XXX should I use TIMER2? */
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	struct ixpclk_softc*	sc;

	sc = ixpclk_sc;
	stathz = profhz = 0;

	printf("clock: hz = %d stathz = %d\n", hz, stathz);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CONTROL,
			  IXPCL_DISABLE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CLEAR, IXPT_CLEAR);

	ixp12x0_intr_establish(IXPPCI_INTR_T1, IPL_CLOCK, ixpclk_intr, NULL);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_LOAD,
			  sc->sc_clock_count);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCLK_CONTROL,
			  IXPCL_ENABLE | IXPCL_PERIODIC
			  | IXPCL_STP_CORE);
}

int
gettick(void)
{
	int	counter;
	u_int	savedints;

	savedints = disable_interrupts(I32_bit);
	counter = GET_TIMER_VALUE(ixpclk_sc);
	restore_interrupts(savedints);
	return counter;
}

static u_int
ixpclk_get_timecount(struct timecounter *tc)
{
	u_int	savedints, base, counter;

	savedints = disable_interrupts(I32_bit);
	do {
		base = ixpclk_base;
		counter = GET_TIMER_VALUE(ixpclk_sc);
	} while (base != ixpclk_base);
	restore_interrupts(savedints);

	return base - counter;
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(unsigned int usecs)
{
	u_int32_t	count;
	u_int32_t	ticks;
	u_int32_t	otick;
	u_int32_t	delta;
	int		j;
	int		csec;
	int		usec;

	if (ixpclk_sc == NULL) {
#ifdef DEBUG
		printf("delay: called befor start ixpclk\n");
#endif

		csec = usecs / 10000;
		usec = usecs % 10000;
	
		usecs = (TIMER_FREQUENCY / 100) * csec
			+ (TIMER_FREQUENCY / 100) * usec / 10000;
		/* clock isn't initialized yet */
		for(; usecs > 0; usecs--)
			for(j = 100; j > 0; j--)
				;
		return;
	}

	count = ixpclk_sc->sc_count_per_usec * usecs;

	otick = gettick();

	for (;;) {
		for(j = 100; j > 0; j--)
			;

		ticks = gettick();
		delta = otick < ticks
			? ixpclk_sc->sc_clock_count + otick - ticks
			: otick - ticks;

		if (delta > count)
			break;

		count -= delta;
		otick = ticks;
	}
}
