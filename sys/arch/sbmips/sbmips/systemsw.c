/* $NetBSD: systemsw.c,v 1.9 2003/07/15 03:35:51 lukem Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: systemsw.c,v 1.9 2003/07/15 03:35:51 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <mips/locore.h>
#include <machine/intr.h>
#include <machine/systemsw.h>

/* trivial functions for function switch */
static void	clock_init_triv(void *);
static uint32_t	clkread_triv(void);
static void	cpu_intr_triv(uint32_t, uint32_t, uint32_t, uint32_t);
static void	cpu_setsoftintr_triv(void);
static void	delay_triv(u_long);
static void	inittodr_triv(void *, time_t);
static void	microtime_triv(struct timeval *);
static void	resettodr_triv(void *);

/* system function switch */
struct systemsw systemsw = {
	cpu_intr_triv,
	cpu_setsoftintr_triv,
	microtime_triv,
	clkread_triv,
	delay_triv,

	NULL,			/* clock intr arg */
	clock_init_triv,

	NULL,			/* statclock arg */
	NULL,			/* s_statclock_init: dflt no-op */
	NULL,			/* s_statclock_setrate: dflt no-op */

	NULL,			/* todr functions arg */
	inittodr_triv,
	resettodr_triv,
	NULL,			/* intr_establish */
};

int
system_set_clockfns(void *arg, void (*init)(void *))
{

	if (systemsw.s_clock_init != clock_init_triv)
		return 1;
	systemsw.s_clock_arg = arg;
	systemsw.s_clock_init = init;
	return 0;
}

int
system_set_todrfns(void *arg, void (*init)(void *, time_t),
    void (*reset)(void *))
{

	if (systemsw.s_inittodr != inittodr_triv ||
	    systemsw.s_resettodr != resettodr_triv)
		return 1;
	systemsw.s_todr_arg = arg;
	systemsw.s_inittodr = init;
	systemsw.s_resettodr = reset;
	return 0;
}


/* trivial microtime() implementation */
static void
microtime_triv(struct timeval *tvp)
{
	int s;
	static struct timeval lasttime;

	s = splclock();
	*tvp = time;

	if (systemsw.s_clkread)
		tvp->tv_usec += systemsw.s_clkread();

	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
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

static uint32_t
clkread_triv(void)
{
	uint32_t res, count;

	count = mips3_cp0_count_read();
	MIPS_COUNT_TO_MHZ(curcpu(), count, res);
	return (res);
}


/* trivial delay() implementation */
static void
delay_triv(u_long n)
{
	u_long i;
	long divisor = curcpu()->ci_divisor_delay;

	while (n-- > 0)
		for (i = divisor; i > 0; i--)
			;
}

static void
cpu_intr_triv(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{

	panic("cpu_intr_triv");
}

void
cpu_setsoftintr_triv(void)
{

	panic("cpu_setsoftintr_triv");
}

void
cpu_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{

	(*systemsw.s_cpu_intr)(status, cause, pc, ipending);
}

void
microtime(struct timeval *tvp)
{

	(*systemsw.s_microtime)(tvp);
}

static void
clock_init_triv(void *arg)
{

	panic("clock_init_triv");
}

static void
inittodr_triv(void *arg, time_t t)
{

	time.tv_sec = t;
}

static void
resettodr_triv(void *arg)
{

	/* do nothing */
}

void
cpu_initclocks(void)
{

	(*systemsw.s_clock_init)(systemsw.s_clock_arg);

	if (systemsw.s_statclock_init != NULL)
		(*systemsw.s_statclock_init)(systemsw.s_statclock_arg);

	/*
	 * ``Disable'' the compare interrupt by setting it to it's largest
	 * value.  The counter will be reset to 0 every hz.  This is used
	 * for microtime.
	 */
	mips3_cp0_compare_write(~(uint32_t)0);
	mips3_cp0_count_write(0);
}

void
setstatclockrate(int hzrate)
{

	if (systemsw.s_statclock_setrate != NULL)
		(*systemsw.s_statclock_setrate)(systemsw.s_statclock_arg,
		    hzrate);
}

void
inittodr(time_t t)
{

	(*systemsw.s_inittodr)(systemsw.s_todr_arg, t);
}

void
resettodr(void)
{

	(*systemsw.s_resettodr)(systemsw.s_todr_arg);
}
