/*	$NetBSD: clock.c,v 1.6.4.1 1997/03/12 14:22:11 is Exp $	*/

/*
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
 * Machine-dependent clock routines for the Mostek48t02
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <m68k/asm_single.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/obio.h>
#include <machine/machdep.h>

#include <dev/clock_subr.h>

#include <sun3/sun3/interreg.h>
#include "mostek48t02.h"

#define	CLOCK_PRI	5
#define IREG_CLK_BITS	(IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5)

void _isr_clock __P((void));	/* in locore.s */
void clock_intr __P((struct clockframe));

static volatile void *clock_va;

static int  clock_match __P((struct device *, struct cfdata *, void *args));
static void clock_attach __P((struct device *, struct device *, void *));

struct cfattach clock_ca = {
	sizeof(struct device), clock_match, clock_attach
};

struct cfdriver clock_cd = {
	NULL, "clock", DV_DULL
};


/*
 * This is called very early (by obio_init()) but after
 * intreg_init() has found the PROM mapping for the
 * interrupt register and cleared it.
 */
void
clock_init()
{
	/* Yes, use the EEPROM address.  It is the same H/W device. */
	clock_va = obio_find_mapping(OBIO_EEPROM, sizeof(struct clockreg));
	if (!clock_va) {
		mon_printf("clock_init\n");
		sunmon_abort();
	}
}

/*
 * XXX  Need to determine which type of clock we have!
 * XXX  The Sun3/80 always has the MK4802, while the
 * XXX  Sun3/470 can (reportedly) have that or the old
 * XXX  intersil7170.  Should have two clock drivers...
 */
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

	/* Validate the given address. */
	if (ca->ca_paddr != OBIO_CLOCK2)
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

	printf("\n");

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
	register u_char interreg;

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

#ifdef	SUN3_470
	if (intersil_va) {
		/*
		 * Then disable clock interrupts, and read the clock's
		 * interrupt register to clear any pending signals there.
		 */
		intersil_clock->clk_cmd_reg =
			intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE);
		intersil_clear();
	}
#endif	/* SUN3_470 */

	/* Set the requested bits in the interrupt register. */
	single_inst_bset_b(*interrupt_reg, interreg);

#ifdef	SUN3_470
	/* Turn the clock back on (maybe) */
	if (intersil_va && enable_clk)
		intersil_clock->clk_cmd_reg =
			intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);
#endif	/* SUN3_470 */

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
	isr_add_custom(5, (void*)_isr_clock);

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

	/* Pulse the clock intr. enable low. */
	single_inst_bclr_b(*interrupt_reg, IREG_CLOCK_ENAB_5);
	single_inst_bset_b(*interrupt_reg, IREG_CLOCK_ENAB_5);

	/* Call common clock interrupt handler. */
	hardclock(&cf);

	/* No LED frobbing on the 3/80 */
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
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
		tvp->tv_usec <= lasttime.tv_usec &&
		(tvp->tv_usec = lasttime.tv_usec + 1) > 1000000)
	{
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

static long clk_get_secs(void);
static void clk_set_secs(long);

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
		fs_time = 21*SECYR + 186*SECDAY + SECDAY/2;
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
		if (diff >= (SECDAY*2)) {
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
 * Routines to copy state into and out of the clock.
 * The clock CSR has to be set for read or write.
 */
static void
clk_get_dt(struct clock_ymdhms *dt)
{
	volatile struct clockreg *cl = clock_va;
	int s;

	s = splhigh();

	/* enable read (stop time) */
	cl->cl_csr |= CLK_READ;

	/* Copy the info */
	dt->dt_sec  = cl->cl_sec;
	dt->dt_min  = cl->cl_min;
	dt->dt_hour = cl->cl_hour;
	dt->dt_wday = cl->cl_wday;
	dt->dt_day  = cl->cl_mday;
	dt->dt_mon  = cl->cl_month;
	dt->dt_year = cl->cl_year;

	/* Done reading (time wears on) */
	cl->cl_csr &= ~CLK_READ;
	splx(s);
}

static void
clk_set_dt(struct clock_ymdhms *dt)
{
	volatile struct clockreg *cl = clock_va;
	int s;

	s = splhigh();
	/* enable write */
	cl->cl_csr |= CLK_WRITE;

	/* Copy the info */
	cl->cl_sec = dt->dt_sec;
	cl->cl_min = dt->dt_min;
	cl->cl_hour = dt->dt_hour;
	cl->cl_wday = dt->dt_wday;
	cl->cl_mday = dt->dt_day;
	cl->cl_month = dt->dt_mon;
	cl->cl_year = dt->dt_year;

	/* load them up */
	cl->cl_csr &= ~CLK_WRITE;
	splx(s);
}


/*
 * Now routines to get and set clock as POSIX time.
 * Our clock keeps "years since 1/1/1968".
 */
#define	CLOCK_BASE_YEAR 1968

static long
clk_get_secs()
{
	struct clock_ymdhms dt;
	long secs;

	clk_get_dt(&dt);

	/* Convert BCD values to binary. */
	dt.dt_sec  = FROMBCD(dt.dt_sec);
	dt.dt_min  = FROMBCD(dt.dt_min);
	dt.dt_hour = FROMBCD(dt.dt_hour);
	dt.dt_day  = FROMBCD(dt.dt_day);
	dt.dt_mon  = FROMBCD(dt.dt_mon);
	dt.dt_year = FROMBCD(dt.dt_year);

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

	/* Convert binary values to BCD. */
	dt.dt_sec  = TOBCD(dt.dt_sec);
	dt.dt_min  = TOBCD(dt.dt_min);
	dt.dt_hour = TOBCD(dt.dt_hour);
	dt.dt_day  = TOBCD(dt.dt_day);
	dt.dt_mon  = TOBCD(dt.dt_mon);
	dt.dt_year = TOBCD(dt.dt_year);

	clk_set_dt(&dt);
}
