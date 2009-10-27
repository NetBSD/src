/*$NetBSD: at91st.c,v 1.3 2009/10/27 03:42:31 snj Exp $*/

/*
 * AT91RM9200 clock functions
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
__KERNEL_RCSID(0, "$NetBSD: at91st.c,v 1.3 2009/10/27 03:42:31 snj Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91streg.h>

#include <opt_hz.h>     /* for HZ */


//#define DEBUG_CLK
#ifdef DEBUG_CLK
#define DPRINTF(fmt...)  printf(fmt)
#else
#define DPRINTF(fmt...)  
#endif


static int at91st_match(device_t, cfdata_t, void *);
static void at91st_attach(device_t, device_t, void *);

void rtcinit(void);

/* callback functions for intr_functions */
static int at91st_intr(void* arg);

struct at91st_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
	int		sc_pid;
	int		sc_initialized;
};

static struct at91st_softc *at91st_sc = NULL;
static struct timeval lasttv;

    
    
/* Match value for clock timer; running at 32.768kHz, want HZ ticks per second  */
/* BTW, we use HZ == 64 or HZ == 128 so have a nice divisor                 */
/* NOTE: don't change there without visiting the functions below which      */
/* convert between timer counts and microseconds                            */
#define AT91ST_DIVIDER	(AT91_SCLK / HZ)
#define USEC_PER_TICK	(1000000 / (AT91_SCLK / AT91ST_DIVIDER))

#if 0
static uint32_t at91st_count_to_usec(uint32_t count)
{
    uint32_t result;
    
    /* convert specified number of ticks to usec, and round up  */
    /* note that with 16 kHz tick rate, maximum count will be   */
    /* 256 (for HZ = 64), so we won't have overflow issues      */ 
    result = (1000000 * count) / AT91_SCLK;

    if ((result * AT91_SCLK) != (count * 1000000))
    {
        /* round up */
        result += 1;
    }
    
    return result;
}

/* This may only be called when overflow is avoided; typically, */
/* it will be used when usec < USEC_PER_TICK              */
static uint32_t usec_to_timer_count(uint32_t usec)
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
#define READ_ST(offset)	STREG(offset)
//bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset)
#define WRITE_ST(offset, value) do {	\
  STREG(offset) = (value);			\
} while (/*CONSTCOND*/0)
//bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, value)



CFATTACH_DECL(at91st, sizeof(struct at91st_softc), at91st_match, at91st_attach, NULL, NULL);



static int
at91st_match(device_t parent, cfdata_t match, void *aux)
{
    if (strcmp(match->cf_name, "at91st") == 0)
	return 2;
    return 0;
}

static void
at91st_attach(device_t parent, device_t self, void *aux)
{
    struct at91st_softc *sc = (struct at91st_softc*) self;
    struct at91bus_attach_args *sa = (struct at91bus_attach_args*) aux;

    printf("\n");
    
    sc->sc_iot = sa->sa_iot;
    sc->sc_pid = sa->sa_pid;
    
#if 0
    DPRINTF("-> bus_space_map()\n");

    /* map bus space and get handle */
    if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0, &sc->sc_ioh) != 0)
        panic("%s: Cannot map registers", self->dv_xname);
#endif

    if (at91st_sc == NULL)
        at91st_sc = sc;

    at91_peripheral_clock(sc->sc_pid, 1);

    WRITE_ST(ST_IDR, -1);	/* make sure interrupts are disabled	*/

    /* set up and enable interval timer 1 as kernel timer, */
    /* using 32kHz clock source */
    WRITE_ST(ST_PIMR, AT91ST_DIVIDER);
    WRITE_ST(ST_RTMR, 1);

    sc->sc_initialized = 1;

    DPRINTF("%s: done\n", __FUNCTION__);

}

/*
 * at91st_intr:
 *
 *Handle the hardclock interrupt.
 */
static int
at91st_intr(void *arg)
{
//    struct at91st_softc *sc = at91st_sc;

    /* make sure it's the kernel timer that generated the interrupt  */
    /* need to do this since the interrupt line is shared by the    */
    /* other interval and PWM timers                                */
    if (READ_ST(ST_SR) & ST_SR_PITS)
    {
        /* call the kernel timer handler */
        hardclock((struct clockframe*) arg);
#if 0
        if (hardclock_ticks % (HZ * 10) == 0)
            printf("time %i sec\n", hardclock_ticks/HZ);
#endif
        return 1;
    }
    else
    {
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
    struct at91st_softc *sc = at91st_sc;

    if (!sc || !sc->sc_initialized)
	panic("%s: driver has not been initialized! (sc=%p)", __FUNCTION__, sc);

    stathz = profhz = 0;

    /* set up and enable interval timer 1 as kernel timer, */
    /* using 32kHz clock source */
    WRITE_ST(ST_PIMR, AT91ST_DIVIDER);

    /* register interrupt handler */
    at91_intr_establish(sc->sc_pid, IPL_CLOCK, INTR_HIGH_LEVEL, at91st_intr, NULL);

    /* enable interrupts from timer */
    WRITE_ST(ST_IER, ST_SR_PITS);
}




/*
 * microtime:
 *
 *Fill in the specified timeval struct with the current time
 *accurate to the microsecond.
 */
void
microtime(register struct timeval *tvp)
{
//    struct at91st_softc *sc = at91st_sc;
    u_int oldirqstate;
    u_int current_count;
    
#ifdef DEBUG
    if (at91st_sc == NULL) {
        printf("microtime: called before initialize at91st\n");
        tvp->tv_sec = 0;
        tvp->tv_usec = 0;
        return;
    }
#endif

    oldirqstate = disable_interrupts(I32_bit);
    
    /* get current timer count */
    current_count = READ_ST(ST_CRTR);

    /* Fill in the timeval struct. */
    *tvp = time;

#if 0
    /* Refine the usec field using current timer count */
    tvp->tv_usec += at91st_count_to_usec(AT91ST_DIVIDER - current_count);
    
    /* Make sure microseconds doesn't overflow. */
    while (__predict_false(tvp->tv_usec >= 1000000)) 
    {
        tvp->tv_usec -= 1000000;
        tvp->tv_sec++;
    }
#endif

    /* Make sure the time has advanced. */
    if (__predict_false(tvp->tv_sec == lasttv.tv_sec && tvp->tv_usec <= lasttv.tv_usec)) 
    {
        tvp->tv_usec = lasttv.tv_usec + 1;
        if (tvp->tv_usec >= 1000000) 
        {
            tvp->tv_usec -= 1000000;
            tvp->tv_sec++;
        }
    }

    lasttv = *tvp;
    
    restore_interrupts(oldirqstate);
}


#if 0
extern int hardclock_ticks;
static void tdelay(unsigned int ticks)
{
    u_int32_t   start, end, current;
    
    current = hardclock_ticks;
    start = current;
    end = start + ticks;
    
    /* just loop for the specified number of ticks */
    while (current < end)
        current = hardclock_ticks;
}
#endif

static void udelay(unsigned int usec)
{
//    struct at91st_softc *sc = at91st_sc;
    u_int32_t crtv, t, diff;

    usec = (usec * 1000 + AT91_SCLK - 1) / AT91_SCLK + 1;

    for (crtv = READ_ST(ST_CRTR);;) {
      while (crtv == (t = READ_ST(ST_CRTR))) ;
      diff = (t - crtv) & ST_CRTR_CRTV;
      if (diff >= usec) {
	break;
      }
      crtv = t;
      usec -= diff;
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

#ifdef DEBUG
    if (at91st_sc == NULL) {
        printf("delay: called before start at91st\n");
        return;
    }
#endif

    if (usec >= USEC_PER_TICK)
    {
        /* have more than 1 tick; just do in ticks */
        unsigned int ticks = usec / USEC_PER_TICK;
        if (ticks*USEC_PER_TICK != usec)
            ticks += 1;
        while (ticks-- > 0) {
	  udelay(USEC_PER_TICK);
	}
    }
    else
    {
        /* less than 1 tick; can do as usec */
        udelay(usec);
    }
    
}

