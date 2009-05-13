/* $NetBSD: s3c2800_clk.c,v 1.14.10.1 2009/05/13 17:16:18 jym Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2800_clk.c,v 1.14.10.1 2009/05/13 17:16:18 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/s3c2xx0/s3c2800reg.h>
#include <arm/s3c2xx0/s3c2800var.h>


#ifndef STATHZ
#define STATHZ	64
#endif

#define TIMER_FREQUENCY(pclk) ((pclk)/32) /* divider=1/32 */

static unsigned int timer0_reload_value;
static unsigned int timer0_prescaler;
static unsigned int timer0_mseccount;

#define usec_to_counter(t)	\
	((timer0_mseccount*(t))/1000)

#define counter_to_usec(c,pclk)	\
	(((c)*timer0_prescaler*1000)/(TIMER_FREQUENCY(pclk)/1000))

static u_int	s3c2800_get_timecount(struct timecounter *);

static struct timecounter s3c2800_timecounter = {
	s3c2800_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	0,		/* frequency */
	"s3c23800",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static volatile uint32_t s3c2800_base;

static u_int
s3c2800_get_timecount(struct timecounter *tc)
{
	struct s3c2800_softc *sc = (struct s3c2800_softc *) s3c2xx0_softc;
	int save, int_pend0, int_pend1, count;

	save = disable_interrupts(I32_bit);

 again:
	int_pend0 = S3C2800_INT_TIMER0 &
	    bus_space_read_4(sc->sc_sx.sc_iot, sc->sc_sx.sc_intctl_ioh,
		INTCTL_SRCPND);
	count = bus_space_read_2(sc->sc_sx.sc_iot, sc->sc_tmr0_ioh,
	    TIMER_TMCNT);
	
	for (;;){

		int_pend1 = S3C2800_INT_TIMER0 &
		    bus_space_read_4(sc->sc_sx.sc_iot, sc->sc_sx.sc_intctl_ioh,
			INTCTL_SRCPND);
		if( int_pend0 == int_pend1 )
			break;

		/*
		 * Down counter reached to zero while we were reading
		 * timer values. do it again to get consistent values.
		 */
		int_pend0 = int_pend1;
		count = bus_space_read_2(sc->sc_sx.sc_iot, sc->sc_tmr0_ioh,
		    TIMER_TMCNT);
	}

	if( __predict_false(count > timer0_reload_value) ){
		/* 
		 * Buggy Hardware Warning --- sometimes timer counter
		 * reads bogus value like 0xffff.  I guess it happens when
		 * the timer is reloaded.
		 */
#if 0
		printf( "Bogus value from timer counter: %d\n", count );
#endif
		goto again;
	}

	restore_interrupts(save);

	if (int_pend1)
		count -= timer0_reload_value;

	return s3c2800_base - count;
}

static inline int
read_timer(struct s3c2800_softc *sc)
{
	int count;

	do {
		count = bus_space_read_2(sc->sc_sx.sc_iot, sc->sc_tmr0_ioh,
		    TIMER_TMCNT);
	} while ( __predict_false(count > timer0_reload_value) );

	return count;
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(u_int n)
{
	struct s3c2800_softc *sc = (struct s3c2800_softc *) s3c2xx0_softc;
	int v0, v1, delta;
	u_int ucnt;

	if ( timer0_reload_value == 0 ){
		/* not initialized yet */
		while ( n-- > 0 ){
			int m;

			for (m=0; m<100; ++m )
				;
		}
		return;
	}

	/* read down counter */
	v0 = read_timer(sc);

	ucnt = usec_to_counter(n);

	while( ucnt > 0 ) {
		v1 = read_timer(sc);
		delta = v0 - v1;
		if ( delta < 0 )
			delta += timer0_reload_value;
#ifdef DEBUG
		if (delta < 0 || delta > timer0_reload_value)
			panic("wrong value from timer counter");
#endif

		if((u_int)delta < ucnt){
			ucnt -= (u_int)delta;
			v0 = v1;
		}
		else {
			ucnt = 0;
		}
	}
	/*NOTREACHED*/
}

void
setstatclockrate(int newhz)
{
}

static int
hardintr(void *arg)
{
	atomic_add_32(&s3c2800_base, timer0_reload_value);

	hardclock((struct clockframe *)arg);

	return 1;
}

static int
statintr(void *arg)
{
	statclock((struct clockframe *)arg);

	return 1;
}

void
cpu_initclocks(void)
{
	struct s3c2800_softc *sc = (struct s3c2800_softc *)s3c2xx0_softc;
	long tc;
	int prescaler;
	int pclk = s3c2xx0_softc->sc_pclk;

	stathz = STATHZ;
	profhz = stathz;

#define calc_time_constant(hz)					\
	do {							\
		prescaler = 1;					\
		do {						\
			++prescaler;				\
			tc = TIMER_FREQUENCY(pclk) /(hz)/ prescaler;	\
		} while( tc > 65536 );				\
	} while(0)



	/* Use the channels 0 and 1 for hardclock and statclock, respectively */
	bus_space_write_4(sc->sc_sx.sc_iot, sc->sc_tmr0_ioh, TIMER_TMCON, 0);
	bus_space_write_4(sc->sc_sx.sc_iot, sc->sc_tmr1_ioh, TIMER_TMCON, 0);

	calc_time_constant(hz);
	bus_space_write_4(sc->sc_sx.sc_iot, sc->sc_tmr0_ioh, TIMER_TMDAT,
	    ((prescaler - 1) << 16) | (tc - 1));
	timer0_prescaler = prescaler;
	timer0_reload_value = tc;
	timer0_mseccount = TIMER_FREQUENCY(pclk)/timer0_prescaler/1000 ;

	printf("clock: hz=%d stathz = %d PCLK=%d prescaler=%d tc=%ld\n",
	    hz, stathz, pclk, prescaler, tc);

	calc_time_constant(stathz);
	bus_space_write_4(sc->sc_sx.sc_iot, sc->sc_tmr1_ioh, TIMER_TMDAT,
	    ((prescaler - 1) << 16) | (tc - 1));


	s3c2800_intr_establish(S3C2800_INT_TIMER0, IPL_CLOCK, 
			       IST_NONE, hardintr, 0);
	s3c2800_intr_establish(S3C2800_INT_TIMER1, IPL_HIGH,
			       IST_NONE, statintr, 0);

	/* start timers */
	bus_space_write_4(sc->sc_sx.sc_iot, sc->sc_tmr0_ioh, TIMER_TMCON, 
	    TMCON_MUX_DIV32|TMCON_INTENA|TMCON_ENABLE);
	bus_space_write_4(sc->sc_sx.sc_iot, sc->sc_tmr1_ioh, TIMER_TMCON, 
	    TMCON_MUX_DIV4|TMCON_INTENA|TMCON_ENABLE);

	/* stop timer2 */
	{
		bus_space_handle_t tmp_ioh;

		bus_space_map(sc->sc_sx.sc_iot, S3C2800_TIMER2_BASE,
		    S3C2800_TIMER_SIZE, 0, &tmp_ioh);

		bus_space_write_4(sc->sc_sx.sc_iot, tmp_ioh,
		    TIMER_TMCON, 0);

		bus_space_unmap(sc->sc_sx.sc_iot, tmp_ioh,
		    S3C2800_TIMER_SIZE);

	}

	s3c2800_timecounter.tc_frequency = TIMER_FREQUENCY(pclk) / timer0_prescaler;
	tc_init(&s3c2800_timecounter);
}
