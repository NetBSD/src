/*	$NetBSD: loongson_clock.c,v 1.1.18.2 2017/12/03 11:36:09 jdolecek Exp $	*/

/*
 * Copyright (c) 2011, 2016 Michael Lorenz
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: loongson_clock.c,v 1.1.18.2 2017/12/03 11:36:09 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/timetc.h>
#include <sys/sysctl.h>

#include <mips/mips3_clock.h>
#include <mips/locore.h>
#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#ifdef LOONGSON_CLOCK_DEBUG
#define DPRINTF aprint_error
#else
#define DPRINTF while (0) printf
#endif

static uint32_t sc_last;
static uint32_t sc_scale[8];
static uint32_t sc_count;	/* should probably be 64 bit */
static int sc_step = 7;
static int sc_step_wanted = 7;
static void *sc_shutdown_cookie;

/* 0, 1/4, 3/8, 1/2, 5/8, 3/4, 7/8, 1 */
static int scale_m[] = {1, 1, 3, 1, 5, 3, 7, 1};
static int scale_d[] = {0, 4, 8, 2, 8, 4, 8, 1};
static int cycles[8];

#define scale(x, f) (x * scale_d[f] / scale_m[f])
#define rscale(x, f) (x * scale_m[f] / scale_d[f])

static void loongson_set_speed(int);
static int  loongson_cpuspeed_temp(SYSCTLFN_ARGS);
static int  loongson_cpuspeed_cur(SYSCTLFN_ARGS);
static int  loongson_cpuspeed_available(SYSCTLFN_ARGS);

static void loongson_clock_shutdown(void *);
static u_int get_loongson_timecount(struct timecounter *);
void	    loongson_delay(int);
void	    loongson_setstatclockrate(int);
void        loongson_initclocks(void);

static struct timecounter loongson_timecounter = {
	get_loongson_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	0,			/* frequency */
	"loongson",		/* name */
	100,			/* quality */
	NULL,			/* tc_priv */
	NULL			/* tc_next */
};

void
loongson_initclocks(void)
{
	const struct sysctlnode *sysctl_node, *me, *freq;
	int clk;

	/*
	 * Establish a hook so on shutdown we can set the CPU clock back to
	 * full speed. This is necessary because PMON doesn't change the 
	 * clock scale register on a warm boot, the MIPS clock code gets
	 * confused if we're too slow and the loongson-specific bits run
	 * too late in the boot process
	 */
	sc_shutdown_cookie = shutdownhook_establish(loongson_clock_shutdown, NULL);

	for (clk = 1; clk < 8; clk++) {
		sc_scale[clk] = rscale(curcpu()->ci_cpu_freq / 1000000, clk);
		cycles[clk] =
		    (rscale(curcpu()->ci_cpu_freq, clk) + hz / 2) / (2 * hz);
	}
#ifdef LOONGSON_CLOCK_DEBUG
	for (clk = 1; clk < 8; clk++) {
		aprint_normal("frequencies: %d/8: %d\n", clk + 1,
		    sc_scale[clk]);
	}
#endif

	/* now setup sysctl */
	if (sysctl_createv(NULL, 0, NULL, 
	    &me, 
	    CTLFLAG_READWRITE, CTLTYPE_NODE, "loongson", NULL, NULL,
	    0, NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL) != 0)
		aprint_error("couldn't create 'loongson' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &freq, 
	    CTLFLAG_READWRITE, CTLTYPE_NODE, "frequency", NULL, NULL, 0, NULL,
	    0, CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL) != 0)
		aprint_error("couldn't create 'frequency' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "target", "CPU speed", loongson_cpuspeed_temp, 
	    0, NULL, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		aprint_error("couldn't create 'target' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "current", NULL, loongson_cpuspeed_cur, 
	    1, NULL, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		aprint_error("couldn't create 'current' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE,
	    CTLTYPE_STRING, "available", NULL, loongson_cpuspeed_available, 
	    2, NULL, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		aprint_error("couldn't create 'available' node\n");

	sc_count = 0;
	loongson_timecounter.tc_frequency = curcpu()->ci_cpu_freq / 2;
	curcpu()->ci_cctr_freq = loongson_timecounter.tc_frequency;

	sc_last = mips3_cp0_count_read();
	mips3_cp0_compare_write(sc_last + curcpu()->ci_cycles_per_hz);

	tc_init(&loongson_timecounter);

	/*
	 * Now we can enable all interrupts including hardclock(9)
	 * by CPU INT5.
	 */
	spl0();
	printf("boom\n");
}

static void
loongson_clock_shutdown(void *cookie)
{

	/* just in case the interrupt handler runs again after this */
	sc_step_wanted = 7;
	/* set the clock to full speed */
	REGVAL(LS2F_CHIPCFG0) =
	    (REGVAL(LS2F_CHIPCFG0) & ~LS2FCFG_FREQSCALE_MASK) | 7;
}

void
loongson_set_speed(int speed)
{

	if ((speed < 1) || (speed > 7))
		return;
	sc_step_wanted = speed;
	DPRINTF("%s: %d\n", __func__, speed);
}

/*
 * the clock interrupt handler
 * we don't have a CPU clock independent, high resolution counter so we're
 * stuck with a PWM that can't count and a CP0 counter that slows down or
 * speeds up with the actual CPU speed. In order to still get halfway
 * accurate time we do the following:
 * - only change CPU speed in the timer interrupt
 * - each timer interrupt we measure how many CP0 cycles passed since last
 *   time, adjust for CPU speed since we can be sure it didn't change, use
 *   that to update a separate counter
 * - when reading the time counter we take the number of CP0 ticks since 
 *   the last timer interrupt, scale it to CPU clock, return that plus the
 *   interrupt updated counter mentioned above to get something close to
 *   CP0 running at full speed 
 * - when changing CPU speed do it as close to taking the time from CP0 as
 *   possible to keep the period of time we spend with CP0 running at the
 *   wrong frequency as short as possible - hopefully short enough to stay
 *   insignificant compared to other noise since switching speeds isn't
 *   going to happen all that often
 */

void
mips3_clockintr(struct clockframe *cf)
{
	uint32_t now, diff, next, new_cnt;

	/*
	 * this looks kinda funny but what we want here is this:
	 * - reading the counter and changing the CPU clock should be as
	 *   close together as possible in order to remain halfway accurate
	 * - we need to use the previous sc_step in order to scale the
	 *   interval passed since the last clock interrupt correctly, so
	 *   we only change sc_step after doing that
	 */
	if (sc_step_wanted != sc_step) {

		REGVAL(LS2F_CHIPCFG0) =
		    (REGVAL(LS2F_CHIPCFG0) & ~LS2FCFG_FREQSCALE_MASK) |
		     sc_step_wanted;
	}

	now = mips3_cp0_count_read();		
	diff = now - sc_last;
	sc_count += scale(diff, sc_step);
	sc_last = now;
	if (sc_step_wanted != sc_step) {
		sc_step = sc_step_wanted;
		curcpu()->ci_cycles_per_hz = cycles[sc_step];
	}
	next = now + curcpu()->ci_cycles_per_hz;
	curcpu()->ci_ev_count_compare.ev_count++;

	mips3_cp0_compare_write(next);

	/* Check for lost clock interrupts */
	new_cnt = mips3_cp0_count_read();

	/* 
	 * Missed one or more clock interrupts, so let's start 
	 * counting again from the current value.
	 */
	if ((next - new_cnt) & 0x80000000) {

		next = new_cnt + curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next);
		curcpu()->ci_ev_count_compare_missed.ev_count++;
	}
 
	hardclock(cf);
}

static u_int
get_loongson_timecount(struct timecounter *tc)
{
	uint32_t now, diff;

	now = mips3_cp0_count_read();
	diff = now - sc_last;
	return sc_count + scale(diff, sc_step);
}

static int
loongson_cpuspeed_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int mhz, i;

	mhz = sc_scale[sc_step_wanted];

	node.sysctl_data = &mhz;
	if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
		int new_reg;

		new_reg = *(int *)node.sysctl_data;
		i = 1;
		while ((i < 8) && (sc_scale[i] != new_reg))
			i++;
		if (i > 7)
			return EINVAL;
		loongson_set_speed(i);
		return 0;
	}
	return EINVAL;
}

static int
loongson_cpuspeed_cur(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int mhz;

	mhz = sc_scale[sc_step];
	node.sysctl_data = &mhz;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
loongson_cpuspeed_available(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	char buf[128];

	snprintf(buf, 128, "%d %d %d %d %d %d %d", sc_scale[1],
	    sc_scale[2], sc_scale[3], sc_scale[4],
	    sc_scale[5], sc_scale[6], sc_scale[7]);
	node.sysctl_data = buf;
	return(sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Wait for at least "n" microseconds.
 */
void
loongson_delay(int n)
{
	u_long divisor_delay;
	uint32_t cur, last, delta, usecs;

	last = mips3_cp0_count_read();
	delta = usecs = 0;

	divisor_delay = rscale(curcpu()->ci_divisor_delay, sc_step);
	if (divisor_delay == 0) {
		/*
		 * Frequency values in curcpu() are not initialized.
		 * Assume faster frequency since longer delays are harmless.
		 * Note CPU_MIPS_DOUBLE_COUNT is ignored here.
		 */
#define FAST_FREQ	(300 * 1000 * 1000)	/* fast enough? */
		divisor_delay = FAST_FREQ / (1000 * 1000);
	}

	while (n > usecs) {
		cur = mips3_cp0_count_read();

		/*
		 * The MIPS3 CP0 counter always counts upto UINT32_MAX,
		 * so no need to check wrapped around case.
		 */
		delta += (cur - last);

		last = cur;

		while (delta >= divisor_delay) {
			/*
			 * delta is not so larger than divisor_delay here,
			 * and using DIV/DIVU ops could be much slower.
			 * (though longer delay may be harmless)
			 */
			usecs++;
			delta -= divisor_delay;
		}
	}
}

SYSCTL_SETUP(sysctl_ams_setup, "sysctl obio subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
loongson_setstatclockrate(int newhz)
{

	/* nothing we can do */
}

__weak_alias(setstatclockrate, loongson_setstatclockrate);
__weak_alias(cpu_initclocks, loongson_initclocks);
__weak_alias(delay, loongson_delay);