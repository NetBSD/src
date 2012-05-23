/*	$NetBSD: iomd_clock.c,v 1.25.14.2 2012/05/23 10:07:42 yamt Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * clock.c
 *
 * Timer related machine specific code
 *
 * Created      : 29/09/94
 */

/* Include header files */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: iomd_clock.c,v 1.25.14.2 2012/05/23 10:07:42 yamt Exp $");

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <dev/clock_subr.h>

#include <arm/cpufunc.h>

#include <arm/iomd/iomdvar.h>
#include <arm/iomd/iomdreg.h>

struct clock_softc {
	device_t 		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

#define TIMER_FREQUENCY 2000000		/* 2MHz clock */
#define TICKS_PER_MICROSECOND (TIMER_FREQUENCY / 1000000)

static void *clockirq;
static void *statclockirq;
static struct clock_softc *clock_sc;
static int timer0_count;

static int clockmatch(device_t parent, cfdata_t cf, void *aux);
static void clockattach(device_t parent, device_t self, void *aux);
#ifdef DIAGNOSTIC
static void checkdelay(void);
#endif

static u_int iomd_timecounter0_get(struct timecounter *tc);

static volatile uint32_t timer0_lastcount;
static volatile uint32_t timer0_offset;
static volatile int timer0_ticked;
/* TODO: Get IRQ status */

static kmutex_t tmr_lock;

static struct timecounter iomd_timecounter = {
	iomd_timecounter0_get,
	0, /* No poll_pps */
	~0, /* 32bit accuracy */
	TIMER_FREQUENCY,
	"iomd_timer0",
	100 
};

int clockhandler(void *);
int statclockhandler(void *);

CFATTACH_DECL_NEW(clock, sizeof(struct clock_softc),
    clockmatch, clockattach, NULL, NULL);

/*
 * int clockmatch(device_t parent, void *match, void *aux)
 *
 * Just return ok for this if it is device 0
 */ 
 
static int
clockmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct clk_attach_args *ca = aux;

	if (strcmp(ca->ca_name, "clk") == 0)
		return(1);
	return(0);
}


/*
 * void clockattach(device_t parent, device_t dev, void *aux)
 *
 * Map the IOMD and identify it.
 * Then configure the child devices based on the IOMD ID.
 */
  
static void
clockattach(device_t parent, device_t self, void *aux)
{
	struct clock_softc *sc = device_private(self);
	struct clk_attach_args *ca = aux;

	sc->sc_dev = self;
	sc->sc_iot = ca->ca_iot;
	sc->sc_ioh = ca->ca_ioh; /* This is a handle for the whole IOMD */

	clock_sc = sc;
	mutex_init(&tmr_lock, MUTEX_DEFAULT, IPL_CLOCK);

	/* Cannot do anything until cpu_initclocks() has been called */

	aprint_normal("\n");
}


static void
tickle_tc(void)
{
	if (timer0_count && 
	    timecounter->tc_get_timecount == iomd_timecounter0_get) {
		mutex_spin_enter(&tmr_lock);
		if (timer0_ticked)
			timer0_ticked    = 0;
		else {
			timer0_offset   += timer0_count;
			timer0_lastcount = 0;
		}
		mutex_spin_exit(&tmr_lock);
	}

}


/*
 * int clockhandler(struct clockframe *frame)
 *
 * Function called by timer 0 interrupts. This just calls
 * hardclock(). Eventually the irqhandler can call hardclock() directly
 * but for now we use this function so that we can debug IRQ's
 */
 
int
clockhandler(void *cookie)
{
	struct clockframe *frame = cookie;
	tickle_tc();

	hardclock(frame);
	return 0;	/* Pass the interrupt on down the chain */
}


/*
 * int statclockhandler(struct clockframe *frame)
 *
 * Function called by timer 1 interrupts. This just calls
 * statclock(). Eventually the irqhandler can call statclock() directly
 * but for now we use this function so that we can debug IRQ's
 */
 
int
statclockhandler(void *cookie)
{
	struct clockframe *frame = cookie;

	statclock(frame);
	return 0;	/* Pass the interrupt on down the chain */
}


/*
 * void setstatclockrate(int newhz)
 *
 * Set the stat clock rate. The stat clock uses timer1
 */

void
setstatclockrate(int newhz)
{
	int count;
    
	count = TIMER_FREQUENCY / newhz;

	aprint_normal("Setting statclock to %dHz (%d ticks)\n", newhz, count);

	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T1LOW, (count >> 0) & 0xff);
	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T1HIGH, (count >> 8) & 0xff);

	/* reload the counter */

	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T1GO, 0);
}


#ifdef DIAGNOSTIC
static void
checkdelay(void)
{
	struct timeval start, end, diff;

	microtime(&start);
	delay(10000);
	microtime(&end);
	timersub(&end, &start, &diff);
	if (diff.tv_sec > 0)
		return;
	if (diff.tv_usec > 10000)
		return;
	aprint_normal("WARNING: delay(10000) took %d us\n", diff.tv_usec);
}
#endif

/*
 * void cpu_initclocks(void)
 *
 * Initialise the clocks.
 * This sets up the two timers in the IOMD and installs the IRQ handlers
 *
 * NOTE: Currently only timer 0 is setup and the IRQ handler is not installed
 */
 
void
cpu_initclocks(void)
{
	/*
	 * Load timer 0 with count down value
	 * This timer generates 100Hz interrupts for the system clock
	 */

	aprint_normal("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

	timer0_count = TIMER_FREQUENCY / hz;

	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T0LOW, (timer0_count >> 0) & 0xff);
	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T0HIGH, (timer0_count >> 8) & 0xff);

	/* reload the counter */

	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T0GO, 0);

	clockirq = intr_claim(IRQ_TIMER0, IPL_CLOCK, "tmr0 hard clk",
	    clockhandler, 0);

	if (clockirq == NULL)
		panic("%s: Cannot installer timer 0 IRQ handler",
		    device_xname(clock_sc->sc_dev));

	if (stathz) {
		setstatclockrate(stathz);
       		statclockirq = intr_claim(IRQ_TIMER1, IPL_CLOCK,
       		    "tmr1 stat clk", statclockhandler, 0);
		if (statclockirq == NULL)
			panic("%s: Cannot installer timer 1 IRQ handler",
			    device_xname(clock_sc->sc_dev));
	}
#ifdef DIAGNOSTIC
	checkdelay();
#endif
	tc_init(&iomd_timecounter);
}



static u_int iomd_timecounter0_get(struct timecounter *tc)
{
	int s;
	u_int tm;

	/*
	 * Latch the current value of the timer and then read it.
	 * This guarantees an atomic reading of the time.
	 */
	s = splhigh();
	bus_space_write_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T0LATCH, 0);
 
	tm = bus_space_read_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T0LOW);
	tm += (bus_space_read_1(clock_sc->sc_iot, clock_sc->sc_ioh,
	    IOMD_T0HIGH) << 8);
	splx(s);

	mutex_spin_enter(&tmr_lock);
	tm = timer0_count - tm;

	if (timer0_count &&
	    (tm < timer0_lastcount || (!timer0_ticked && false/* XXX: clkintr_pending */))) {
		timer0_ticked = 1;
		timer0_offset += timer0_count;
	}

	timer0_lastcount = tm;
	tm += timer0_offset;
	mutex_spin_exit(&tmr_lock);

	return tm;
}



/*
 * Estimated loop for n microseconds
 */

/* Need to re-write this to use the timers */

/* One day soon I will actually do this */

int delaycount = 100;

void
delay(u_int n)
{
	volatile u_int n2;
	volatile u_int i;

	if (n == 0) return;
	n2 = n;
	while (n2-- > 0) {
		if (cputype == CPU_ID_SA110)	/* XXX - Seriously gross hack */
			for (i = delaycount; --i;);
		else
			for (i = 8; --i;);
	}
}

/* End of iomd_clock.c */
