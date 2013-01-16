/*	$NetBSD: epclk.c,v 1.18.2.2 2013/01/16 05:32:46 yamt Exp $	*/

/*
 * Copyright (c) 2004 Jesse Off
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

/*
 * Driver for the ep93xx clock tick.
 *
 * We use the 64Hz RTC interrupt as its the only thing that allows for timekeeping
 * of a second (crystal error only).  There are two general purpose timers
 * on the ep93xx, but they run at a frequency that makes a perfect integer 
 * number of ticks per second impossible.  Note that there was an errata with
 * the ep93xx processor and many early boards (including the Cirrus eval board) have
 * a broken crystal oscillator input that may make this 64Hz unreliable.  However,
 * not all boards are susceptible, the Technologic Systems TS-7200 is a notable
 * exception that is immune to this errata.   --joff
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epclk.c,v 1.18.2.2 2013/01/16 05:32:46 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epclkreg.h> 
#include <arm/ep93xx/ep93xxreg.h> 
#include <arm/ep93xx/ep93xxvar.h>
#include <dev/clock_subr.h>

#include "opt_hz.h"

#define	TIMER_FREQ	983040

static int	epclk_match(device_t, cfdata_t, void *);
static void	epclk_attach(device_t, device_t, void *);
static u_int	epclk_get_timecount(struct timecounter *);

void		rtcinit(void);

/* callback functions for intr_functions */
static int      epclk_intr(void* arg);

struct epclk_softc {
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
#if defined(HZ) && (HZ == 64)
	bus_space_handle_t	sc_teoi_ioh;
#endif
	int			sc_intr;
};

static struct timecounter epclk_timecounter = {
	epclk_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	TIMER_FREQ,		/* frequency */
	"epclk",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static struct epclk_softc *epclk_sc = NULL;

CFATTACH_DECL_NEW(epclk, sizeof(struct epclk_softc),
    epclk_match, epclk_attach, NULL, NULL);

/* This is a quick ARM way to multiply by 983040/1000000 (w/o overflow) */
#define US_TO_TIMER4VAL(x, y) { \
	uint32_t hi, lo, scalar = 4222124650UL; \
	__asm volatile ( \
		"umull %0, %1, %2, %3;" \
		: "=&r"(lo), "=&r"(hi) \
		: "r"((x)), "r"(scalar) \
	); \
	(y) = hi; \
}

#define TIMER4VAL()	(*(volatile uint32_t *)(EP93XX_APB_VBASE + \
	EP93XX_APB_TIMERS + EP93XX_TIMERS_Timer4ValueLow))

static int
epclk_match(device_t parent, cfdata_t match, void *aux)
{

	return 2;
}

static void
epclk_attach(device_t parent, device_t self, void *aux)
{
	struct epclk_softc		*sc;
	struct epsoc_attach_args	*sa;
	bool first_run;

	printf("\n");

	sc = device_private(self);
	sa = aux;
	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;
	sc->sc_intr = sa->sa_intr;

	if (epclk_sc == NULL) {
		first_run = true;
		epclk_sc = sc;
	}

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 
		0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));
#if defined(HZ) && (HZ == 64)
	if (bus_space_map(sa->sa_iot, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON + 
		EP93XX_SYSCON_TEOI, 4, 0, &sc->sc_teoi_ioh))
		panic("%s: Cannot map registers", device_xname(self));
#endif

	/* clear and start the debug timer (Timer4) */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_TIMERS_Timer4Enable, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_TIMERS_Timer4Enable, 0x100);

	if (first_run)
		tc_init(&epclk_timecounter);
}

/*
 * epclk_intr:
 *
 *	Handle the hardclock interrupt.
 */
static int
epclk_intr(void *arg)
{
	struct epclk_softc*	sc;

	sc = epclk_sc;

#if defined(HZ) && (HZ == 64)
	bus_space_write_4(sc->sc_iot, sc->sc_teoi_ioh, 0, 1);
#else
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_TIMERS_Timer1Clear, 1);
#endif
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
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	struct epclk_softc*	sc;

	sc = epclk_sc;
	stathz = profhz = 0;

#if defined(HZ) && (HZ == 64)
	if (hz != 64) panic("HZ must be 64!");

	/* clear 64Hz interrupt status */
	bus_space_write_4(sc->sc_iot, sc->sc_teoi_ioh, 0, 1);
#else
#define	CLOCK_SOURCE_RATE	14745600UL
#define	CLOCK_TICK_DIV		29
#define	CLOCK_TICK_RATE \
	(((CLOCK_SOURCE_RATE+(CLOCK_TICK_DIV*hz-1))/(CLOCK_TICK_DIV*hz))*hz)
#define	LATCH	((CLOCK_TICK_RATE + hz/2) / hz)
	/* setup and start the 16bit timer (Timer1) */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  EP93XX_TIMERS_Timer1Control,
			  (TimerControl_MODE)|(TimerControl_CLKSEL));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  EP93XX_TIMERS_Timer1Load, LATCH-1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  EP93XX_TIMERS_Timer1Control,
			  (TimerControl_ENABLE)|(TimerControl_MODE)|(TimerControl_CLKSEL));
#endif

	ep93xx_intr_establish(sc->sc_intr, IPL_CLOCK, epclk_intr, NULL);
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(unsigned int n)
{
	unsigned int cur_tick, initial_tick;
	int remaining;

#ifdef DEBUG
	if (epclk_sc == NULL) {
		printf("delay: called before start epclk\n");
		return;
	}
#endif

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	initial_tick = TIMER4VAL();

	US_TO_TIMER4VAL(n, remaining);

	while (remaining > 0) {
		cur_tick = TIMER4VAL();
		if (cur_tick >= initial_tick)
			remaining -= cur_tick - initial_tick;
		else
			remaining -= UINT_MAX - initial_tick + cur_tick + 1;
		initial_tick = cur_tick;
	}
}

static u_int
epclk_get_timecount(struct timecounter *tc)
{
	return TIMER4VAL();
}
