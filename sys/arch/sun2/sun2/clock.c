/*	$NetBSD: clock.c,v 1.1.2.2 2001/04/21 17:54:55 bouyer Exp $	*/

/*
 * Copyright (c) 2001 Matthew Fredette
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: clock.c 1.18 91/01/21$
 *	from: @(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Machine-dependent clock routines for the Am9513
 * Written by Matthew Fredette, based on the sun3 clock.c by 
 * Adam Glass and Gordon Ross.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/leds.h>

#include <sun2/sun2/control.h>
#include <sun2/sun2/enable.h>
#include <sun2/sun2/machdep.h>

#include <dev/clock_subr.h>
#include <dev/ic/am9513reg.h>

/*
 * Carefully define the basic CPU clock rate so
 * that time-of-day calculations don't float
 *
 * Note that the CLK_BASIC is divided by 4 before we can count with it,
 * e.g. F1 ticks CLK_BASIC/4 times a second.
 */
#define	SUN2_CLK_BASIC		(19660800)
#define SUN2_CLK_TICKS(func, hz) (((SUN2_CLK_BASIC / 4) / AM9513_CM_SOURCE_Fn_DIV(func)) / (hz))

/* These define which counters are used for what. */
#define	SUN2_CLK_NMI		AM9513_TIMER1	/* Non Maskable Interrupts */
#define	SUN2_CLK_TIMER		AM9513_TIMER2	/* Timer 2 */
#define	SUN2_CLK_UNUSED		AM9513_TIMER3	/* Unused timer */
#define	SUN2_CLK_FAST_LO	AM9513_TIMER4	/* Timer 4 for realtime, low  order */
#define	SUN2_CLK_FAST_HI	AM9513_TIMER5	/* Timer 5 for realtime, high order */

#define	CLOCK_PRI	5
#define IREG_CLK_BITS	(IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5)

void _isr_clock __P((void));	/* in locore.s */
void clock_intr __P((struct clockframe));

static bus_space_tag_t am9513_bt;
static bus_space_handle_t am9513_bh;
#define am9513_write_clk_cmd(val) bus_space_write_2(am9513_bt, am9513_bh, AM9513_CLK_CMD, val)
#define am9513_write_clk_data(val) bus_space_write_2(am9513_bt, am9513_bh, AM9513_CLK_DATA, val)

static int  clock_match __P((struct device *, struct cfdata *, void *args));
static void clock_attach __P((struct device *, struct device *, void *));

struct cfattach clock_ca = {
	sizeof(struct device), clock_match, clock_attach
};

static int
clock_match(parent, cf, args)
    struct device *parent;
	struct cfdata *cf;
    void *args;
{
	struct confargs *ca = args;

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return (0);

	/* Make sure there is something there... */
	if (!bus_space_probe(ca->ca_bustag, 0, ca->ca_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL))
		return (0);

	/* Default interrupt priority. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = CLOCK_PRI;

	return (1);
}

static void
clock_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct confargs *ca = args;
	bus_space_handle_t bh;

	printf("\n");

	/* Get a mapping for it. */
	if (bus_space_map(ca->ca_bustag, ca->ca_paddr, sizeof(struct am9513), 0, &bh))
		panic("clock_attach");
	am9513_bt = ca->ca_bustag;
	am9513_bh = bh;

	/*
	 * Set the clock to the correct interrupt rate, but
	 * do not enable the interrupt until cpu_initclocks.
	 */

	/* Disarm the timer and NMI. */
	am9513_write_clk_cmd(AM9513_CMD_DISARM(SUN2_CLK_TIMER | SUN2_CLK_NMI));
	am9513_write_clk_cmd(AM9513_CMD_CLEAR_OUTPUT(SUN2_CLK_TIMER));
	am9513_write_clk_cmd(AM9513_CMD_CLEAR_OUTPUT(SUN2_CLK_NMI));

	/* Set the clock to 100 Hz, but do not enable it yet. */
	am9513_write_clk_cmd(AM9513_CMD_LOAD_MODE(SUN2_CLK_TIMER));
	am9513_write_clk_data((AM9513_CM_MODE_D
			       | AM9513_CM_SOURCE_F2
			       | AM9513_CM_OUTPUT_TC_TOGGLED));
	am9513_write_clk_cmd(AM9513_CMD_LOAD_LOAD(SUN2_CLK_TIMER));
	am9513_write_clk_data(SUN2_CLK_TICKS(AM9513_CM_SOURCE_F2, 100));

	/*
	 * Can not hook up the ISR until cpu_initclocks()
	 * because hardclock is not ready until then.
	 * For now, the handler is _isr_autovec(), which
	 * will complain if it gets clock interrupts.
	 */
}

/*
 * Set or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 * XXX:  Watch out!  It's really easy to break this!
 */
void
set_clk_mode(prom_clock, on)
	int prom_clock;
	int on;
{
	int timer;

#ifdef	DIAGNOSTIC
	/* Assertion: were are at splhigh! */
	if ((getsr() & PSL_IPL) < PSL_IPL7)
		panic("set_clk_mode: bad ipl");
#endif

	/* Get the timer we're talking about. */
	timer = (prom_clock ? SUN2_CLK_NMI : SUN2_CLK_TIMER);

	/* First, turn off the "master" enable bit. */
	enable_reg_and(~ENA_INTS);

	/*
	 * Arm the timer we're supposed to turn on.
	 */
	if (on) {
		am9513_write_clk_cmd(AM9513_CMD_ARM(timer));
	}

	/*
	 * Disarm and clear the timers we're supposed to turn off.
	 */
	else {
		am9513_write_clk_cmd(AM9513_CMD_DISARM(timer));
		am9513_write_clk_cmd(AM9513_CMD_CLEAR_OUTPUT(timer));
	}

	/* Finally, turn the "master" enable back on. */
	enable_reg_or(ENA_INTS);
}

/*
 * Set up the real-time clock (enable clock interrupts).
 * Leave stathz 0 since there is no secondary clock available.
 * Note that clock interrupts MUST STAY DISABLED until here.
 */
void
cpu_initclocks(void)
{
	int s;

	s = splhigh();

	/* Install isr (in locore.s) that calls clock_intr(). */
	isr_add_custom(5, (void*)_isr_clock);

	/* Now enable the clock at level 5 in the interrupt reg. */
	set_clk_mode(0, 1);

	splx(s);
}

/*
 * This doesn't need to do anything, as we have only one timer and
 * profhz==stathz==hz.
 */
void
setstatclockrate(newhz)
	int newhz;
{
	/* nothing */
}

/*
 * This is is called by the "custom" interrupt handler.
 * Note that we can get ZS interrupts while this runs,
 * and zshard may touch the interrupt_reg, so we must
 * be careful to use the single_inst_* macros to modify
 * the interrupt register atomically.
 */
void
clock_intr(cf)
	struct clockframe cf;
{

	/* Read the clock interrupt register. */
	am9513_write_clk_cmd(AM9513_CMD_CLEAR_OUTPUT(SUN2_CLK_TIMER));

	{ /* Entertainment! */
#ifdef	LED_IDLE_CHECK
		/* With this option, LEDs move only when CPU is idle. */
		extern char _Idle[];	/* locore.s */
		if (cf.cf_pc == (long)_Idle)
#endif
			leds_intr();
	}

	/* Call common clock interrupt handler. */
	hardclock(&cf);
}


/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt.
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for
 * fun, we guarantee that the time will be greater than the value
 * obtained by a previous call.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec++; 	/* XXX */
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
		tvp->tv_usec <= lasttime.tv_usec &&
		(tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000)
	{
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}
