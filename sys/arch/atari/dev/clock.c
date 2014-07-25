/*	$NetBSD: clock.c,v 1.59 2014/07/25 08:10:32 dholland Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	7.6 (Berkeley) 5/7/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.59 2014/07/25 08:10:32 dholland Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/event.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/dev/clockreg.h>
#include <atari/dev/clockvar.h>
#include <atari/atari/device.h>

#if defined(GPROF) && defined(PROFTIMER)
#include <machine/profile.h>
#endif

#include "ioconf.h"

static int	atari_rtc_get(todr_chip_handle_t, struct clock_ymdhms *);
static int	atari_rtc_set(todr_chip_handle_t, struct clock_ymdhms *);

/*
 * The MFP clock runs at 2457600Hz. We use a {system,stat,prof}clock divider
 * of 200. Therefore the timer runs at an effective rate of:
 * 2457600/200 = 12288Hz.
 */
#define CLOCK_HZ	12288

static u_int clk_getcounter(struct timecounter *);

static struct timecounter clk_timecounter = {
	clk_getcounter,	/* get_timecount */
	0,		/* no poll_pps */
	~0u,		/* counter_mask */
	CLOCK_HZ,	/* frequency */
	"clock",	/* name, overriden later */
	100,		/* quality */
	NULL,		/* prev */
	NULL,		/* next */
};

/*
 * Machine-dependent clock routines.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

struct clock_softc {
	device_t	sc_dev;
	int		sc_flags;
	struct todr_chip_handle	sc_handle;
};

/*
 *  'sc_flags' state info. Only used by the rtc-device functions.
 */
#define	RTC_OPEN	1

dev_type_open(rtcopen);
dev_type_close(rtcclose);
dev_type_read(rtcread);
dev_type_write(rtcwrite);

static void	clockattach(device_t, device_t, void *);
static int	clockmatch(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(clock, sizeof(struct clock_softc),
    clockmatch, clockattach, NULL, NULL);

const struct cdevsw rtc_cdevsw = {
	.d_open = rtcopen,
	.d_close = rtcclose,
	.d_read = rtcread,
	.d_write = rtcwrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

void statintr(struct clockframe);

static int	twodigits(char *, int);

static int	divisor;	/* Systemclock divisor	*/

/*
 * Statistics and profile clock intervals and variances. Variance must
 * be a power of 2. Since this gives us an even number, not an odd number,
 * we discard one case and compensate. That is, a variance of 64 would
 * give us offsets in [0..63]. Instead, we take offsets in [1..63].
 * This is symmetric around the point 32, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
#ifdef STATCLOCK
static int	statvar = 32;	/* {stat,prof}clock variance		*/
static int	statmin;	/* statclock divisor - variance/2	*/
static int	profmin;	/* profclock divisor - variance/2	*/
static int	clk2min;	/* current, from above choices		*/
#endif

int
clockmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (!strcmp("clock", aux))
		return 1;
	return 0;
}

/*
 * Start the real-time clock.
 */
void clockattach(device_t parent, device_t self, void *aux)
{
	struct clock_softc *sc = device_private(self);
	struct todr_chip_handle	*tch;

	sc->sc_dev = self;
	tch = &sc->sc_handle;
	tch->todr_gettime_ymdhms = atari_rtc_get;
	tch->todr_settime_ymdhms = atari_rtc_set;
	tch->todr_setwen = NULL;

	todr_attach(tch);

	sc->sc_flags = 0;

	/*
	 * Initialize Timer-A in the ST-MFP. We use a divisor of 200.
	 * The MFP clock runs at 2457600Hz. Therefore the timer runs
	 * at an effective rate of: 2457600/200 = 12288Hz. The
	 * following expression works for 48, 64 or 96 hz.
	 */
	divisor       = CLOCK_HZ/hz;
	MFP->mf_tacr  = 0;		/* Stop timer			*/
	MFP->mf_iera &= ~IA_TIMA;	/* Disable timer interrupts	*/
	MFP->mf_tadr  = divisor;	/* Set divisor			*/

	clk_timecounter.tc_frequency = CLOCK_HZ;

	if (hz != 48 && hz != 64 && hz != 96) { /* XXX */
		printf (": illegal value %d for systemclock, reset to %d\n\t",
								hz, 64);
		hz = 64;
	}
	printf(": system hz %d timer-A divisor 200/%d\n", hz, divisor);
	tc_init(&clk_timecounter);

#ifdef STATCLOCK
	if ((stathz == 0) || (stathz > hz) || (CLOCK_HZ % stathz))
		stathz = hz;
	if ((profhz == 0) || (profhz > (hz << 1)) || (CLOCK_HZ % profhz))
		profhz = hz << 1;

	MFP->mf_tcdcr &= 0x7;			/* Stop timer		*/
	MFP->mf_ierb  &= ~IB_TIMC;		/* Disable timer inter.	*/
	MFP->mf_tcdr   = CLOCK_HZ/stathz;	/* Set divisor		*/

	statmin  = (CLOCK_HZ/stathz) - (statvar >> 1);
	profmin  = (CLOCK_HZ/profhz) - (statvar >> 1);
	clk2min  = statmin;
#endif /* STATCLOCK */
}

void cpu_initclocks(void)
{

	MFP->mf_tacr  = T_Q200;		/* Start timer			*/
	MFP->mf_ipra  = (u_int8_t)~IA_TIMA;/* Clear pending interrupts	*/
	MFP->mf_iera |= IA_TIMA;	/* Enable timer interrupts	*/
	MFP->mf_imra |= IA_TIMA;	/*    .....			*/

#ifdef STATCLOCK
	MFP->mf_tcdcr = (MFP->mf_tcdcr & 0x7) | (T_Q200<<4); /* Start	*/
	MFP->mf_iprb  = (u_int8_t)~IB_TIMC;/* Clear pending interrupts	*/
	MFP->mf_ierb |= IB_TIMC;	/* Enable timer interrupts	*/
	MFP->mf_imrb |= IB_TIMC;	/*    .....			*/
#endif /* STATCLOCK */
}

void
setstatclockrate(int newhz)
{

#ifdef STATCLOCK
	if (newhz == stathz)
		clk2min = statmin;
	else clk2min = profmin;
#endif /* STATCLOCK */
}

#ifdef STATCLOCK
void
statintr(struct clockframe frame)
{
	register int	var, r;

	var = statvar - 1;
	do {
		r = random() & var;
	} while (r == 0);

	/*
	 * Note that we are always lagging behind as the new divisor
	 * value will not be loaded until the next interrupt. This
	 * shouldn't disturb the median frequency (I think ;-) ) as
	 * only the value used when switching frequencies is used
	 * twice. This shouldn't happen very often.
	 */
	MFP->mf_tcdr = clk2min + r;

	statclock(&frame);
}
#endif /* STATCLOCK */

static u_int
clk_getcounter(struct timecounter *tc)
{
	uint32_t delta, count, cur_hardclock;
	uint8_t ipra, tadr;
	int s;
	static uint32_t lastcount;

	s = splhigh();
	cur_hardclock = hardclock_ticks;
	ipra = MFP->mf_ipra;
	tadr = MFP->mf_tadr;
	delta = divisor - tadr;

	if (ipra & IA_TIMA)
		delta += divisor;
	splx(s);

	count = (divisor * cur_hardclock) + delta;
	if ((int32_t)(count - lastcount) < 0) {
		/* XXX wrapped; maybe hardclock() is blocked more than 2/HZ */
		count = lastcount + 1;
	}
	lastcount = count;

	return count;
}

#define TIMB_FREQ	614400
#define TIMB_LIMIT	256

void
init_delay(void)
{

	/*
	 * Initialize Timer-B in the ST-MFP. This timer is used by
	 * the 'delay' function below. This timer is setup to be
	 * continueously counting from 255 back to zero at a
	 * frequency of 614400Hz. We do this *early* in the
	 * initialisation process.
	 */
	MFP->mf_tbcr  = 0;		/* Stop timer			*/
	MFP->mf_iera &= ~IA_TIMB;	/* Disable timer interrupts	*/
	MFP->mf_tbdr  = 0;	
	MFP->mf_tbcr  = T_Q004;	/* Start timer			*/
}

/*
 * Wait "n" microseconds.
 * Relies on MFP-Timer B counting down from TIMB_LIMIT at TIMB_FREQ Hz.
 * Note: timer had better have been programmed before this is first used!
 */
void
delay(unsigned int n)
{
	int	ticks, otick, remaining;

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	otick = MFP->mf_tbdr;

	if (n <= UINT_MAX / TIMB_FREQ) {
		/*
		 * For unsigned arithmetic, division can be replaced with
		 * multiplication with the inverse and a shift.
		 */
		remaining = n * TIMB_FREQ / 1000000;
	} else {
		/* This is a very long delay.
		 * Being slow here doesn't matter.
		 */
		remaining = (unsigned long long) n * TIMB_FREQ / 1000000;
	}

	while (remaining > 0) {
		ticks = MFP->mf_tbdr;
		if (ticks > otick)
			remaining -= TIMB_LIMIT - (ticks - otick);
		else
			remaining -= otick - ticks;
		otick = ticks;
	}
}

#ifdef GPROF
/*
 * profclock() is expanded in line in lev6intr() unless profiling kernel.
 * Assumes it is called with clock interrupts blocked.
 */
profclock(void *pc, int ps)
{

	/*
	 * Came from user mode.
	 * If this process is being profiled record the tick.
	 */
	if (USERMODE(ps)) {
		if (p->p_stats.p_prof.pr_scale)
			addupc(pc, &curproc->p_stats.p_prof, 1);
	}
	/*
	 * Came from kernel (supervisor) mode.
	 * If we are profiling the kernel, record the tick.
	 */
	else if (profiling < 2) {
		register int s = pc - s_lowpc;

		if (s < s_textsize)
			kcount[s / (HISTFRACTION * sizeof(*kcount))]++;
	}
	/*
	 * Kernel profiling was on but has been disabled.
	 * Mark as no longer profiling kernel and if all profiling done,
	 * disable the clock.
	 */
	if (profiling && (profon & PRF_KERNEL)) {
		profon &= ~PRF_KERNEL;
		if (profon == PRF_NONE)
			stopprofclock();
	}
}
#endif

/***********************************************************************
 *                   Real Time Clock support                           *
 ***********************************************************************/

u_int mc146818_read(void *cookie, u_int regno)
{
	struct rtc *rtc = cookie;

	rtc->rtc_regno = regno;
	return rtc->rtc_data & 0xff;
}

void mc146818_write(void *cookie, u_int regno, u_int value)
{
	struct rtc *rtc = cookie;

	rtc->rtc_regno = regno;
	rtc->rtc_data  = value;
}

static int
atari_rtc_get(todr_chip_handle_t todr, struct clock_ymdhms *dtp)
{
	int			sps;
	mc_todregs		clkregs;
	u_int			regb;

	sps = splhigh();
	regb = mc146818_read(RTC, MC_REGB);
	MC146818_GETTOD(RTC, &clkregs);
	splx(sps);

	regb &= MC_REGB_24HR|MC_REGB_BINARY;
	if (regb != (MC_REGB_24HR|MC_REGB_BINARY)) {
		printf("Error: Nonstandard RealTimeClock Configuration -"
			" value ignored\n"
			"       A write to /dev/rtc will correct this.\n");
			return 0;
	}
	if (clkregs[MC_SEC] > 59)
		return -1;
	if (clkregs[MC_MIN] > 59)
		return -1;
	if (clkregs[MC_HOUR] > 23)
		return -1;
	if (range_test(clkregs[MC_DOM], 1, 31))
		return -1;
	if (range_test(clkregs[MC_MONTH], 1, 12))
		return -1;
	if (clkregs[MC_YEAR] > 99)
		return -1;

	dtp->dt_year = clkregs[MC_YEAR] + GEMSTARTOFTIME;
	dtp->dt_mon  = clkregs[MC_MONTH];
	dtp->dt_day  = clkregs[MC_DOM];
	dtp->dt_hour = clkregs[MC_HOUR];
	dtp->dt_min  = clkregs[MC_MIN];
	dtp->dt_sec  = clkregs[MC_SEC];

	return 0;
}

static int
atari_rtc_set(todr_chip_handle_t todr, struct clock_ymdhms *dtp)
{
	int s;
	mc_todregs clkregs;

	clkregs[MC_YEAR] = dtp->dt_year - GEMSTARTOFTIME;
	clkregs[MC_MONTH] = dtp->dt_mon;
	clkregs[MC_DOM] = dtp->dt_day;
	clkregs[MC_HOUR] = dtp->dt_hour;
	clkregs[MC_MIN] = dtp->dt_min;
	clkregs[MC_SEC] = dtp->dt_sec;

	s = splclock();
	MC146818_PUTTOD(RTC, &clkregs);
	splx(s);

	return 0;
}

/***********************************************************************
 *                   RTC-device support				       *
 ***********************************************************************/
int
rtcopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int			unit = minor(dev);
	struct clock_softc	*sc;

	sc = device_lookup_private(&clock_cd, unit);
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_flags & RTC_OPEN)
		return EBUSY;

	sc->sc_flags = RTC_OPEN;
	return 0;
}

int
rtcclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int			unit = minor(dev);
	struct clock_softc	*sc = device_lookup_private(&clock_cd, unit);

	sc->sc_flags = 0;
	return 0;
}

int
rtcread(dev_t dev, struct uio *uio, int flags)
{
	mc_todregs		clkregs;
	int			s, length;
	char			buffer[16 + 1];

	s = splhigh();
	MC146818_GETTOD(RTC, &clkregs);
	splx(s);

	snprintf(buffer, sizeof(buffer), "%4d%02d%02d%02d%02d.%02d\n",
	    clkregs[MC_YEAR] + GEMSTARTOFTIME,
	    clkregs[MC_MONTH], clkregs[MC_DOM],
	    clkregs[MC_HOUR], clkregs[MC_MIN], clkregs[MC_SEC]);

	if (uio->uio_offset > strlen(buffer))
		return 0;

	length = strlen(buffer) - uio->uio_offset;
	if (length > uio->uio_resid)
		length = uio->uio_resid;

	return uiomove((void *)buffer, length, uio);
}

static int
twodigits(char *buffer, int pos)
{
	int result = 0;

	if (buffer[pos] >= '0' && buffer[pos] <= '9')
		result = (buffer[pos] - '0') * 10;
	if (buffer[pos+1] >= '0' && buffer[pos+1] <= '9')
		result += (buffer[pos+1] - '0');
	return result;
}

int
rtcwrite(dev_t dev, struct uio *uio, int flags)
{
	mc_todregs		clkregs;
	int			s, length, error;
	char			buffer[16];

	/*
	 * We require atomic updates!
	 */
	length = uio->uio_resid;
	if (uio->uio_offset || (length != sizeof(buffer)
	  && length != sizeof(buffer) - 1))
		return EINVAL;
	
	if ((error = uiomove((void *)buffer, sizeof(buffer), uio)))
		return error;

	if (length == sizeof(buffer) && buffer[sizeof(buffer) - 1] != '\n')
		return EINVAL;

	s = splclock();
	mc146818_write(RTC, MC_REGB,
	    mc146818_read(RTC, MC_REGB) | MC_REGB_24HR | MC_REGB_BINARY);
	MC146818_GETTOD(RTC, &clkregs);
	splx(s);

	clkregs[MC_SEC]   = twodigits(buffer, 13);
	clkregs[MC_MIN]   = twodigits(buffer, 10);
	clkregs[MC_HOUR]  = twodigits(buffer, 8);
	clkregs[MC_DOM]   = twodigits(buffer, 6);
	clkregs[MC_MONTH] = twodigits(buffer, 4);
	s = twodigits(buffer, 0) * 100 + twodigits(buffer, 2);
	clkregs[MC_YEAR]  = s - GEMSTARTOFTIME; 

	s = splclock();
	MC146818_PUTTOD(RTC, &clkregs);
	splx(s);

	return 0;
}
