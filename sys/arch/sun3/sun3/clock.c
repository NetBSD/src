/*	$NetBSD: clock.c,v 1.52 2003/09/22 16:54:14 tsutsui Exp $	*/

/*
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
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
 * Machine-dependent clock routines for the Intersil 7170:
 * Original by Adam Glass;  partially rewritten by Gordon Ross.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.52 2003/09/22 16:54:14 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <m68k/asm_single.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/leds.h>

#include <sun3/sun3/control.h>
#include <sun3/sun3/interreg.h>
#include <sun3/sun3/machdep.h>

#include <dev/clock_subr.h>
#include <dev/ic/intersil7170.h>

#define	CLOCK_PRI	5
#define IREG_CLK_BITS	(IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5)

void _isr_clock __P((void));	/* in locore.s */
void clock_intr __P((struct clockframe));

static volatile void *intersil_va;

#define intersil_clock ((volatile struct intersil7170 *) intersil_va)

#define intersil_command(run, interrupt) \
	(run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
	 INTERSIL_CMD_NORMAL_MODE)

#define intersil_clear() (void)intersil_clock->clk_intr_reg

static int  clock_match __P((struct device *, struct cfdata *, void *args));
static void clock_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(clock, sizeof(struct device),
    clock_match, clock_attach, NULL, NULL);

static int
clock_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct confargs *ca = args;

	/* This driver only supports one unit. */
	if (intersil_va)
		return (0);

	/* Make sure there is something there... */
	if (bus_peek(ca->ca_bustype, ca->ca_paddr, 1) == -1)
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
	caddr_t va;

	printf("\n");

	/* Get a mapping for it. */
	va = bus_mapin(ca->ca_bustype,
	    ca->ca_paddr, sizeof(struct intersil7170));
	if (!va)
		panic("clock_attach");
	intersil_va = va;

	/*
	 * Set the clock to the correct interrupt rate, but
	 * do not enable the interrupt until cpu_initclocks.
	 * XXX: Actually, the interrupt_reg should be zero
	 * at this point, so the clock interrupts should not
	 * affect us, but we need to set the rate...
	 */
	intersil_clock->clk_cmd_reg =
	    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE);
	intersil_clear();

	/* Set the clock to 100 Hz, but do not enable it yet. */
	intersil_clock->clk_intr_reg = INTERSIL_INTER_CSECONDS;

	/*
	 * Can not hook up the ISR until cpu_initclocks()
	 * because hardclock is not ready until then.
	 * For now, the handler is _isr_autovec(), which
	 * will complain if it gets clock interrupts.
	 */
}

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 * XXX:  Watch out!  It's really easy to break this!
 */
void
set_clk_mode(on, off, enable_clk)
	u_char on, off;
	int enable_clk;
{
	u_char interreg;

	/*
	 * If we have not yet mapped the register,
	 * then we do not want to do any of this...
	 */
	if (!interrupt_reg)
		return;

#ifdef	DIAGNOSTIC
	/* Assertion: were are at splhigh! */
	if ((getsr() & PSL_IPL) < PSL_IPL7)
		panic("set_clk_mode: bad ipl");
#endif

	/*
	 * make sure that we are only playing w/
	 * clock interrupt register bits
	 */
	on  &= IREG_CLK_BITS;
	off &= IREG_CLK_BITS;

	/* First, turn off the "master" enable bit. */
	single_inst_bclr_b(*interrupt_reg, IREG_ALL_ENAB);

	/*
	 * Save the current interrupt register clock bits,
	 * and turn off/on the requested bits in the copy.
	 */
	interreg = *interrupt_reg & IREG_CLK_BITS;
	interreg &= ~off;
	interreg |= on;

	/* Clear the CLK5 and CLK7 bits to clear the flip-flops. */
	single_inst_bclr_b(*interrupt_reg, IREG_CLK_BITS);

	if (intersil_va) {
		/*
		 * Then disable clock interrupts, and read the clock's
		 * interrupt register to clear any pending signals there.
		 */
		intersil_clock->clk_cmd_reg =
		    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE);
		intersil_clear();
	}

	/* Set the requested bits in the interrupt register. */
	single_inst_bset_b(*interrupt_reg, interreg);

	/* Turn the clock back on (maybe) */
	if (intersil_va && enable_clk)
		intersil_clock->clk_cmd_reg =
		    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);

	/* Finally, turn the "master" enable back on. */
	single_inst_bset_b(*interrupt_reg, IREG_ALL_ENAB);
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
	isr_add_custom(CLOCK_PRI, (void *)_isr_clock);

	/* Now enable the clock at level 5 in the interrupt reg. */
	set_clk_mode(IREG_CLOCK_ENAB_5, 0, 1);

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
	intersil_clear();

	/* Pulse the clock intr. enable low. */
	single_inst_bclr_b(*interrupt_reg, IREG_CLOCK_ENAB_5);
	single_inst_bset_b(*interrupt_reg, IREG_CLOCK_ENAB_5);

	/* Read the clock intr. reg. AGAIN! */
	intersil_clear();

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
	struct timeval *tvp;
{
	int s;
	static struct timeval lasttime;

	s = splhigh();
	*tvp = time;
	tvp->tv_usec++; 	/* XXX */
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}


/*
 * Machine-dependent clock routines.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

static long clk_get_secs __P((void));
static void clk_set_secs __P((long));

/*
 * Initialize the time of day register, based on the time base
 * which is, e.g. from a filesystem.
 */
void inittodr(fs_time)
	time_t fs_time;
{
	long diff, clk_time;
	long long_ago = (5 * SECYR);
	int clk_bad = 0;

	/*
	 * Sanity check time from file system.
	 * If it is zero,assume filesystem time is just unknown
	 * instead of preposterous.  Don't bark.
	 */
	if (fs_time < long_ago) {
		/*
		 * If fs_time is zero, assume filesystem time is just
		 * unknown instead of preposterous.  Don't bark.
		 */
		if (fs_time != 0)
			printf("WARNING: preposterous time in file system\n");
		/* 1991/07/01  12:00:00 */
		fs_time = 21 * SECYR + 186 * SECDAY + SECDAY / 2;
	}

	clk_time = clk_get_secs();

	/* Sanity check time from clock. */
	if (clk_time < long_ago) {
		printf("WARNING: bad date in battery clock");
		clk_bad = 1;
		clk_time = fs_time;
	} else {
		/* Does the clock time jive with the file system? */
		diff = clk_time - fs_time;
		if (diff < 0)
			diff = -diff;
		if (diff >= (SECDAY * 2)) {
			printf("WARNING: clock %s %d days",
			    (clk_time < fs_time) ? "lost" : "gained",
			    (int) (diff / SECDAY));
			clk_bad = 1;
		}
	}
	if (clk_bad)
		printf(" -- CHECK AND RESET THE DATE!\n");
	time.tv_sec = clk_time;
}

/*
 * Resettodr restores the time of day hardware after a time change.
 */
void resettodr()
{

	clk_set_secs(time.tv_sec);
}


/*
 * Now routines to get and set clock as POSIX time.
 * Our clock keeps "years since 1/1/1968".
 */
#define	CLOCK_BASE_YEAR 1968
static void intersil_get_dt __P((struct clock_ymdhms *));
static void intersil_set_dt __P((struct clock_ymdhms *));

static long
clk_get_secs()
{
	struct clock_ymdhms dt;
	long secs;

	intersil_get_dt(&dt);

	if ((dt.dt_hour > 24) ||
	    (dt.dt_day  > 31) ||
	    (dt.dt_mon  > 12))
		return (0);

	dt.dt_year += CLOCK_BASE_YEAR;
	secs = clock_ymdhms_to_secs(&dt);
	return (secs);
}

static void
clk_set_secs(secs)
	long secs;
{
	struct clock_ymdhms dt;

	clock_secs_to_ymdhms(secs, &dt);
	dt.dt_year -= CLOCK_BASE_YEAR;

	intersil_set_dt(&dt);
}


/*
 * Routines to copy state into and out of the clock.
 * The intersil registers have to be read or written
 * in sequential order (or so it appears). -gwr
 */
static void
intersil_get_dt(struct clock_ymdhms *dt)
{
	volatile struct intersil_dt *isdt;
	int s;

	isdt = &intersil_clock->counters;
	s = splhigh();

	/* Enable read (stop time) */
	intersil_clock->clk_cmd_reg =
	    intersil_command(INTERSIL_CMD_STOP, INTERSIL_CMD_IENABLE);

	/* Copy the info.  Careful about the order! */
	dt->dt_sec  = isdt->dt_csec;  /* throw-away */
	dt->dt_hour = isdt->dt_hour;
	dt->dt_min  = isdt->dt_min;
	dt->dt_sec  = isdt->dt_sec;
	dt->dt_mon  = isdt->dt_month;
	dt->dt_day  = isdt->dt_day;
	dt->dt_year = isdt->dt_year;
	dt->dt_wday = isdt->dt_dow;

	/* Done reading (time wears on) */
	intersil_clock->clk_cmd_reg =
	    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);
	splx(s);
}

static void
intersil_set_dt(struct clock_ymdhms *dt)
{
	volatile struct intersil_dt *isdt;
	int s;

	isdt = &intersil_clock->counters;
	s = splhigh();

	/* Enable write (stop time) */
	intersil_clock->clk_cmd_reg =
	    intersil_command(INTERSIL_CMD_STOP, INTERSIL_CMD_IENABLE);

	/* Copy the info.  Careful about the order! */
	isdt->dt_csec = 0;
	isdt->dt_hour = dt->dt_hour;
	isdt->dt_min  = dt->dt_min;
	isdt->dt_sec  = dt->dt_sec;
	isdt->dt_month= dt->dt_mon;
	isdt->dt_day  = dt->dt_day;
	isdt->dt_year = dt->dt_year;
	isdt->dt_dow  = dt->dt_wday;

	/* Done writing (time wears on) */
	intersil_clock->clk_cmd_reg =
	    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);
	splx(s);
}
