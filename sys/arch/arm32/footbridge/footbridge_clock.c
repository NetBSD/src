/*	$NetBSD: footbridge_clock.c,v 1.2.2.4 2001/03/12 13:27:39 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
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
 */

/* Include header files */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <machine/cpufunc.h>
#include <machine/irqhandler.h>
#include <arm32/footbridge/dc21285reg.h>
#include <arm32/footbridge/footbridgevar.h>

extern struct footbridge_softc *clock_sc;
extern u_int dc21285_fclk;

#if 0
static int clockmatch	__P((struct device *parent, struct cfdata *cf, void *aux));
static void clockattach	__P((struct device *parent, struct device *self, void *aux));

struct cfattach footbridge_clock_ca = {
	sizeof(struct clock_softc), clockmatch, clockattach
};

/*
 * int clockmatch(struct device *parent, void *match, void *aux)
 *
 * Just return ok for this if it is device 0
 */ 
 
static int
clockmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union footbridge_attach_args *fba = aux;

	if (strcmp(fba->fba_ca.ca_name, "clk") == 0)
		return(1);
	return(0);
}


/*
 * void clockattach(struct device *parent, struct device *dev, void *aux)
 *
 */
  
static void
clockattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct clock_softc *sc = (struct clock_softc *)self;
	union footbridge_attach_args *fba = aux;

	sc->sc_iot = fba->fba_ca.ca_iot;
	sc->sc_ioh = fba->fba_ca.ca_ioh;

	clock_sc = sc;

	/* Cannot do anything until cpu_initclocks() has been called */
	
	printf("\n");
}
#endif

/*
 * int clockhandler(struct clockframe *frame)
 *
 * Function called by timer 1 interrupts.
 * This just clears the interrupt condition and calls hardclock().
 */

int
clockhandler(frame)
	struct clockframe *frame;
{
	bus_space_write_4(clock_sc->sc_iot, clock_sc->sc_ioh,
	    TIMER_1_CLEAR, 0);
	hardclock(frame);
	return(0);	/* Pass the interrupt on down the chain */
}


/*
 * int statclockhandler(struct clockframe *frame)
 *
 * Function called by timer 2 interrupts.
 * This just clears the interrupt condition and calls statclock().
 */
 
int
statclockhandler(frame)
	struct clockframe *frame;
{
	bus_space_write_4(clock_sc->sc_iot, clock_sc->sc_ioh,
	    TIMER_2_CLEAR, 0);
	statclock(frame);
	return(0);	/* Pass the interrupt on down the chain */
}

static int
load_timer(base, hz)
	int base;
	int hz;
{
	unsigned int timer_count;
	int control;

	timer_count = dc21285_fclk / hz;
	if (timer_count > TIMER_MAX * 16) {
		control = TIMER_FCLK_256;
		timer_count >>= 8;
	} else if (timer_count > TIMER_MAX) {
		control = TIMER_FCLK_16;
		timer_count >>= 4;
	} else
		control = TIMER_FCLK;

	control |= (TIMER_ENABLE | TIMER_MODE_PERIODIC);
	bus_space_write_4(clock_sc->sc_iot, clock_sc->sc_ioh,
	    base + TIMER_LOAD, timer_count);
	bus_space_write_4(clock_sc->sc_iot, clock_sc->sc_ioh,
	    base + TIMER_CONTROL, control);
	bus_space_write_4(clock_sc->sc_iot, clock_sc->sc_ioh,
	    base + TIMER_CLEAR, 0);
	return(timer_count);
}

/*
 * void setstatclockrate(int hz)
 *
 * Set the stat clock rate. The stat clock uses timer2
 */

void
setstatclockrate(hz)
	int hz;
{

	clock_sc->sc_statclock_count = load_timer(TIMER_2_BASE, hz);
}

/*
 * void cpu_initclocks(void)
 *
 * Initialise the clocks.
 *
 * Timer 1 is used for the main system clock (hardclock)
 * Timer 2 is used for the statistics clock (statclock)
 */
 
void
cpu_initclocks()
{

	/* Report the clock frequencies */
	printf("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

	/* Setup timer 1 and claim interrupt */
	clock_sc->sc_clock_count = load_timer(TIMER_1_BASE, hz);

	/*
	 * Use ticks per 256us for accuracy since ticks per us is often
	 * fractional e.g. @ 66MHz
	 */
	clock_sc->sc_clock_ticks_per_256us =
	    ((((clock_sc->sc_clock_count * hz) / 1000) * 256) / 1000);
	clock_sc->sc_clockintr = intr_claim(IRQ_TIMER_1, IPL_CLOCK,
	    "tmr1 hard clk", clockhandler, 0);

	if (clock_sc->sc_clockintr == NULL)
		panic("%s: Cannot install timer 1 interrupt handler\n",
		    clock_sc->sc_dev.dv_xname);

	/* If stathz is non-zero then setup the stat clock */
	if (stathz) {
		/* Setup timer 2 and claim interrupt */
		setstatclockrate(stathz);
       		clock_sc->sc_statclockintr = intr_claim(IRQ_TIMER_2, IPL_CLOCK,
       		    "tmr2 stat clk", statclockhandler, 0);
		if (clock_sc->sc_statclockintr == NULL)
			panic("%s: Cannot install timer 2 interrupt handler\n",
			    clock_sc->sc_dev.dv_xname);
	}
}


/*
 * void microtime(struct timeval *tvp)
 *
 * Fill in the specified timeval struct with the current time
 * accurate to the microsecond.
 */

void
microtime(tvp)
	struct timeval *tvp;
{
	int s;
	int tm;
	int deltatm;
	static struct timeval oldtv;

	if (clock_sc == NULL || clock_sc->sc_clock_count == 0)
		return;

	s = splhigh();

	tm = bus_space_read_4(clock_sc->sc_iot, clock_sc->sc_ioh,
	    TIMER_1_VALUE);

	deltatm = clock_sc->sc_clock_count - tm;

#ifdef DIAGNOSTIC
	if (deltatm < 0)
		panic("opps deltatm < 0 tm=%d deltatm=%d\n", tm, deltatm);
#endif

	/* Fill in the timeval struct */
	*tvp = time;
	tvp->tv_usec += ((deltatm << 8) / clock_sc->sc_clock_ticks_per_256us);

	/* Make sure the micro seconds don't overflow. */
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}

	/* Make sure the time has advanced. */
	if (tvp->tv_sec == oldtv.tv_sec &&
	    tvp->tv_usec <= oldtv.tv_usec) {
		tvp->tv_usec = oldtv.tv_usec + 1;
		if (tvp->tv_usec >= 1000000) {
			tvp->tv_usec -= 1000000;
			++tvp->tv_sec;
		}
	}

	oldtv = *tvp;
	(void)splx(s);		
}

/*
 * Estimated loop for n microseconds
 */

/* Need to re-write this to use the timers */

/* One day soon I will actually do this */

int delaycount = 50;

void
delay(n)
	u_int n;
{
	u_int i;

	if (n == 0) return;
	while (--n > 0) {
		if (cputype == CPU_ID_SA110)	/* XXX - Seriously gross hack */
			for (i = delaycount; --i;);
		else
			for (i = 8; --i;);
	}
}

/* End of footbridge_clock.c */
