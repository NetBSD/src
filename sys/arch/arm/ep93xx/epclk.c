/*	$NetBSD: epclk.c,v 1.8 2005/12/24 22:45:34 perry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: epclk.c,v 1.8 2005/12/24 22:45:34 perry Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epclkreg.h> 
#include <arm/ep93xx/ep93xxreg.h> 
#include <arm/ep93xx/ep93xxvar.h>
#include <dev/clock_subr.h>

#include "opt_hz.h"

static int	epclk_match(struct device *, struct cfdata *, void *);
static void	epclk_attach(struct device *, struct device *, void *);

void		rtcinit(void);

/* callback functions for intr_functions */
static int      epclk_intr(void* arg);

struct epclk_softc {
	struct device		sc_dev;
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
#if defined(HZ) && (HZ == 64)
	bus_space_handle_t	sc_teoi_ioh;
#endif
	int			sc_intr;
};

static struct epclk_softc *epclk_sc = NULL;
static u_int32_t tmark;


/* This is a quick ARM way to multiply by 983040/1000000 */
#define US_TO_TIMER4VAL(x) { \
	u_int32_t hi, lo, scalar = 4222124650UL; \
	__asm volatile ( \
		"umull %0, %1, %2, %3;" \
		: "=&r"(lo), "=&r"(hi) \
		: "r"((x)), "r"(scalar) \
	); \
	(x) = hi; \
}

/* This is a quick ARM way to multiply by 1000000/983040 */
#define TIMER4VAL_TO_US(x) { \
	u_int32_t hi, lo, scalar = 2184533333UL; \
	__asm volatile ( \
		"umull %0, %1, %2, %3;" \
		"mov %1, %1, lsl #1;" \
		"mov %0, %0, lsr #31;" \
		"orr %1, %1, %0;" \
		: "=&r"(lo), "=&r"(hi) \
		: "r"((x)), "r"(scalar) \
	); \
	(x) = hi; \
}


CFATTACH_DECL(epclk, sizeof(struct epclk_softc),
    epclk_match, epclk_attach, NULL, NULL);

#define TIMER4VAL()	(*(volatile u_int32_t *)(EP93XX_APB_VBASE + \
	EP93XX_APB_TIMERS + EP93XX_TIMERS_Timer4ValueLow))

static int
epclk_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 2;
}

static void
epclk_attach(struct device *parent, struct device *self, void *aux)
{
	struct epclk_softc		*sc;
	struct epsoc_attach_args	*sa;

	printf("\n");

	sc = (struct epclk_softc*) self;
	sa = aux;
	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;
	sc->sc_intr = sa->sa_intr;

	if (epclk_sc == NULL)
		epclk_sc = sc;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 
		0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);
#if defined(HZ) && (HZ == 64)
	if (bus_space_map(sa->sa_iot, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON + 
		EP93XX_SYSCON_TEOI, 4, 0, &sc->sc_teoi_ioh))
		panic("%s: Cannot map registers", self->dv_xname);
#endif

	/* clear and start the debug timer (Timer4) */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_TIMERS_Timer4Enable, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_TIMERS_Timer4Enable, 0x100);
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

	tmark = TIMER4VAL();
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
 * microtime:
 *
 *	Fill in the specified timeval struct with the current time
 *	accurate to the microsecond.
 */
void
microtime(register struct timeval *tvp)
{
	u_int			oldirqstate;
	u_int			tmarknow, delta;
	static struct timeval	lasttv;

#ifdef DEBUG
	if (epclk_sc == NULL) {
		printf("microtime: called before initialize epclk\n");
		tvp->tv_sec = 0;
		tvp->tv_usec = 0;
		return;
	}
#endif

	oldirqstate = disable_interrupts(I32_bit);
	tmarknow = TIMER4VAL();

        /* Fill in the timeval struct. */
	*tvp = time;
	if (__predict_false(tmarknow < tmark)) { /* overflow */
		delta = tmarknow + (UINT_MAX - tmark);
	} else {
		delta = tmarknow - tmark;
	}

	TIMER4VAL_TO_US(delta);

	tvp->tv_usec += delta;

        /* Make sure microseconds doesn't overflow. */
	while (__predict_false(tvp->tv_usec >= 1000000)) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

        /* Make sure the time has advanced. */
	if (__predict_false(tvp->tv_sec == lasttv.tv_sec &&
	    tvp->tv_usec <= lasttv.tv_usec)) {
		tvp->tv_usec = lasttv.tv_usec + 1;
		if (tvp->tv_usec >= 1000000) {
			tvp->tv_usec -= 1000000;
			tvp->tv_sec++;
		}
	}

	lasttv = *tvp;

	restore_interrupts(oldirqstate);
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(unsigned int len)
{
	u_int32_t	start, end, ticks;

#ifdef DEBUG
	if (epclk_sc == NULL) {
		printf("delay: called before start epclk\n");
		return;
	}
#endif

	ticks = start = TIMER4VAL();
	US_TO_TIMER4VAL(len);
	end = start + len;
	while (start <= ticks && ticks > end) {
		/* wait for Timer4ValueLow wraparound */
		ticks = TIMER4VAL();
	}
	while (ticks <= end) {
		ticks = TIMER4VAL();
	}
}

todr_chip_handle_t todr_handle;

/*
 * todr_attach:
 *
 *	Set the specified time-of-day register as the system real-time clock.
 */
void
todr_attach(todr_chip_handle_t todr)
{

	if (todr_handle)
		panic("todr_attach: rtc already configured");
	todr_handle = todr;
}

/*
 * inittodr:
 *
 *	Initialize time from the time-of-day register.
 */
#define	MINYEAR		2003	/* minimum plausible year */
void
inittodr(time_t base)
{
	time_t deltat;
	int badbase;

	if (base < (MINYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR;
		badbase = 1;
	} else
		badbase = 0;

	if (todr_handle == NULL ||
	    todr_gettime(todr_handle, &time) != 0 ||
	    time.tv_sec == 0) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		time.tv_sec = base;
		time.tv_usec = 0;
		if (todr_handle != NULL && !badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		goto bad;
	}

	if (!badbase) {
		/*
		 * See if we gained/lost two or more days; if
		 * so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;		/* all is well */
		printf("WARNING: clock %s %ld days\n",
		    time.tv_sec < base ? "lost" : "gained",
		    (long)deltat / SECDAY);
	}
 bad:
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * resettodr:
 *
 *	Reset the time-of-day register with the current time.
 */
void
resettodr(void)
{

	if (time.tv_sec == 0)
		return;

	if (todr_handle != NULL &&
	    todr_settime(todr_handle, &time) != 0)
		printf("resettodr: failed to set time\n");
}
