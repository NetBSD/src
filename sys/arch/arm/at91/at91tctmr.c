/*$NetBSD: at91tctmr.c,v 1.3 2009/10/27 03:42:31 snj Exp $*/

/*
 * AT91 Timer Counter (TC) based clock functions
 * Copyright (c) 2007, Embedtronics Oy
 * All rights reserved.
 *
 * Based on vx115_clk.c,
 * Copyright (c) 2006, Jon Sevy <jsevy@cs.drexel.edu>
 * 
 * Based on epclk.c
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
 * Driver for the AT91RM9200 clock tick.
 * We use Timer 1 for the system clock
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91tctmr.c,v 1.3 2009/10/27 03:42:31 snj Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91tcreg.h>

#include <opt_hz.h>     /* for HZ */


#define DEBUG_CLK
#ifdef DEBUG_CLK
#define DPRINTF(fmt...)  printf(fmt)
#else
#define DPRINTF(fmt...)  
#endif


static int at91tctmr_match(device_t, cfdata_t, void *);
static void at91tctmr_attach(device_t, device_t, void *);

void rtcinit(void);

/* callback functions for intr_functions */
static int at91tctmr_intr(void* arg);

struct at91tctmr_softc {
	device_t	sc_dev;
	u_char		*sc_addr;
	int		sc_pid;
	int		sc_initialized;
	uint32_t	sc_timerclock;
	uint32_t	sc_divider;
	uint32_t	sc_usec_per_tick;
};

static struct at91tctmr_softc *at91tctmr_sc = NULL;
static struct timeval lasttv;

    
    
/* Match value for clock timer; running at master clock, want HZ ticks per second  */
/* NOTE: don't change there without visiting the functions below which      */
/* convert between timer counts and microseconds                            */

static inline uint32_t
at91tctmr_count_to_usec(struct at91tctmr_softc *sc, uint32_t count)
{
    uint64_t tmp;
    
    tmp = count;
    tmp *= 1000000U;

    return (tmp / sc->sc_timerclock);
}

#if 0
/* This may only be called when overflow is avoided; typically, */
/* it will be used when usec < USEC_PER_TICK              */
static uint32_t
usec_to_timer_count(uint32_t usec)
{
    uint32_t result;
    
    /* convert specified number of usec to timer ticks, and round up */
    result = (AT91_SCLK * usec) / 1000000;
    
    if ((result * 1000000) != (usec * AT91_SCLK))
    {
        /* round up */
        result += 1;
    }
    
    return result;
    
}
#endif

/* macros to simplify writing to the timer controller */
static inline u_int32_t
READ_TC(struct at91tctmr_softc *sc, uint offset)
{
	volatile u_int32_t *addr = (void*)(sc->sc_addr + offset);
	return *addr;
}

//bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset)
static inline void
WRITE_TC(struct at91tctmr_softc *sc, uint offset, u_int32_t value)
{
	volatile u_int32_t *addr = (void*)(sc->sc_addr + offset);
	*addr = value;
}


CFATTACH_DECL_NEW(at91tctmr, sizeof(struct at91tctmr_softc),
    at91tctmr_match, at91tctmr_attach, NULL, NULL);

static u_int at91tctmr_get_timecount(struct timecounter *);

static struct timecounter at91tctmr_timecounter = {
	at91tctmr_get_timecount,/* get_timecount */
	0,                      /* no poll_pps */
	0xffffffff,		/* counter_mask */
	COUNTS_PER_SEC,		/* frequency */
	"at91tctmr",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static int
at91tctmr_match(device_t parent, cfdata_t match, void *aux)
{
	if (strcmp(match->cf_name, "at91tctmr") == 0)
		return 2;
	return 0;
}

static void
at91tctmr_attach(device_t parent, device_t self, void *aux)
{
    struct at91tctmr_softc *sc = device_private(self);
    struct at91bus_attach_args *sa = aux;

    aprint_normal("\n");

    sc->sc_dev = self;
    sc->sc_addr = (void*)sa->sa_addr;
    sc->sc_pid = sa->sa_pid;
    
    if (at91tctmr_sc == NULL)
        at91tctmr_sc = sc;

    at91_peripheral_clock(sc->sc_pid, 1);

    WRITE_TC(sc, TC_CCR, TC_CCR_CLKDIS);
    WRITE_TC(sc, TC_IDR, -1);	/* make sure interrupts are disabled	*/

    /* find divider */
    u_int32_t cmr = 0;
    if (AT91_MSTCLK / 2U / HZ <= 65536) {
      sc->sc_timerclock = AT91_MSTCLK / 2U;
      cmr = TC_CMR_TCCLKS_MCK_DIV_2;
    } else if (AT91_MSTCLK / 8U / HZ <= 65536) {
      sc->sc_timerclock = AT91_MSTCLK / 8U;
      cmr = TC_CMR_TCCLKS_MCK_DIV_8;
    } else if (AT91_MSTCLK / 32U / HZ <= 65536) {
      sc->sc_timerclock = AT91_MSTCLK / 32U;
      cmr = TC_CMR_TCCLKS_MCK_DIV_32;
    } else if (AT91_MSTCLK / 128U / HZ <= 65536) {
      sc->sc_timerclock = AT91_MSTCLK / 128U;
      cmr = TC_CMR_TCCLKS_MCK_DIV_128;
    } else
      panic("%s: cannot setup timer to reach HZ", device-xname(sc->sc_dev));

    sc->sc_divider = (sc->sc_timerclock + HZ - 1) / HZ; /* round up */
    sc->sc_usec_per_tick = 1000000UL / (sc->sc_timerclock / sc->sc_divider);

    WRITE_TC(sc, TC_CMR, TC_CMR_WAVE | cmr | TC_CMR_WAVSEL_UP_RC);
    WRITE_TC(sc, TC_CCR, TC_CCR_CLKEN);
    WRITE_TC(sc, TC_RC,  sc->sc_divider - 1);
    WRITE_TC(sc, TC_CCR, TC_CCR_SWTRG);

    sc->sc_initialized = 1;

    DPRINTF("%s: done, tclock=%"PRIu32" div=%"PRIu32" uspertick=%"PRIu32"\n", __FUNCTION__, sc->sc_timerclock, sc->sc_divider, sc->sc_usec_per_tick);

}

/*
 * at91tctmr_intr:
 *
 *Handle the hardclock interrupt.
 */
static int
at91tctmr_intr(void *arg)
{
    struct at91tctmr_softc *sc = arg;

    /* make sure it's the kernel timer that generated the interrupt  */
    /* need to do this since the interrupt line is shared by the    */
    /* other interval and PWM timers                                */
    if (READ_TC(sc, TC_SR) & TC_SR_CPCS) {
        /* call the kernel timer handler */
        hardclock((struct clockframe*) arg);
        return 1;
    } else {
        /* it's one of the other timers; just pass it on */
        return 0;
    }
}

/*
 * setstatclockrate:
 *
 *Set the rate of the statistics clock.
 *
 *We assume that hz is either stathz or profhz, and that neither
 *will change after being set by cpu_initclocks().  We could
 *recalculate the intervals here, but that would be a pain.
 */
void
setstatclockrate(int hzz)
{
        /* use hardclock */
	(void)hzz;
}

/*
 * cpu_initclocks:
 *
 *Initialize the clock and get it going.
 */
static void udelay(unsigned int usec);

void
cpu_initclocks(void)
{
    struct at91tctmr_softc *sc = at91tctmr_sc;

    if (!sc || !sc->sc_initialized)
	panic("%s: driver has not been initialized! (sc=%p)", __FUNCTION__, sc);

    hz = sc->sc_timerclock / sc->sc_divider;
    stathz = profhz = 0;

    /* set up and enable interval timer 1 as kernel timer, */
    /* using 32kHz clock source */

    /* register interrupt handler */
    at91_intr_establish(sc->sc_pid, IPL_CLOCK, INTR_HIGH_LEVEL, at91tctmr_intr, sc);

    /* enable interrupts from timer */
    WRITE_TC(sc, TC_IER, TC_SR_CPCS);
}




static void udelay(unsigned int usec)
{
    struct at91tctmr_softc *sc = at91tctmr_sc;
    u_int32_t prev_cvr, cvr, divi = READ_TC(sc, TC_RC), diff;
    int prev_ticks, ticks, ticks2;
    unsigned footick = (sc->sc_timerclock * 64ULL / 1000000UL);

    if (usec > 0) {
      prev_ticks = hardclock_ticks;
      __insn_barrier();
      prev_cvr = READ_TC(sc, TC_CV);
      ticks = hardclock_ticks;
      __insn_barrier();
      if (ticks != prev_ticks) {
	prev_cvr = READ_TC(sc, TC_CV);
	prev_ticks = ticks;
      }
      for (;;) {
	ticks = hardclock_ticks;
	__insn_barrier();
	cvr = READ_TC(sc, TC_CV);
	ticks2 = hardclock_ticks;
	__insn_barrier();
	if (ticks2 != ticks) {
	  cvr = READ_TC(sc, TC_CV);
	}
	diff = (ticks2 - prev_ticks) * divi;
	if (cvr < prev_cvr) {
	  if (!diff)
	    diff = divi;
	  diff -= prev_cvr - cvr;
	} else
	  diff += cvr - prev_cvr;
	diff = diff * 64 / footick;
	if (diff) {
	  if (usec <= diff)
	    break;
	  prev_ticks = ticks2;
	  prev_cvr = (prev_cvr + footick * diff / 64) % divi;
	  usec -= diff;
	}
      }
    }
}



/*
 * delay:
 *
 *Delay for at least N microseconds. Note that due to our coarse clock,
 *  our resolution is 61 us. But we round up so we'll wait at least as
 *  long as requested.
 */
void
delay(unsigned int usec)
{
    struct at91tctmr_softc *sc = at91tctmr_sc;

#ifdef DEBUG
    if (sc == NULL) {
        printf("delay: called before start at91tc\n");
        return;
    }
#endif

    if (usec >= sc->sc_usec_per_tick) {
        /* have more than 1 tick; just do in ticks */
        unsigned int ticks = (usec + sc->sc_usec_per_tick - 1) / sc->sc_usec_per_tick;
        while (ticks-- > 0) {
	  udelay(sc->sc_usec_per_tick);
	}
    } else {
        /* less than 1 tick; can do as usec */
        udelay(usec);
    }
    
}

