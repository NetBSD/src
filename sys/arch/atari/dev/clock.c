/*	$NetBSD: clock.c,v 1.21 2000/01/06 12:03:31 leo Exp $	*/

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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	7.6 (Berkeley) 5/7/91
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/uio.h>
#include <sys/conf.h>

#include <dev/clock_subr.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/dev/clockreg.h>
#include <atari/atari/device.h>

#if defined(GPROF) && defined(PROFTIMER)
#include <machine/profile.h>
#endif

/*
 * The MFP clock runs at 2457600Hz. We use a {system,stat,prof}clock divider
 * of 200. Therefore the timer runs at an effective rate of:
 * 2457600/200 = 12288Hz.
 */
#define CLOCK_HZ	12288

/*
 * Machine-dependent clock routines.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

struct clock_softc {
	struct device	sc_dev;
	int		sc_flags;
};

/*
 *  'sc_flags' state info. Only used by the rtc-device functions.
 */
#define	RTC_OPEN	1

/* {b,c}devsw[] function prototypes for rtc functions */
dev_type_open(rtcopen);
dev_type_close(rtcclose);
dev_type_read(rtcread);
dev_type_write(rtcwrite);

static void	clockattach __P((struct device *, struct device *, void *));
static int	clockmatch __P((struct device *, struct cfdata *, void *));

struct cfattach clock_ca = {
	sizeof(struct clock_softc), clockmatch, clockattach
};

extern struct cfdriver clock_cd;

void statintr __P((struct clockframe));

static u_long	gettod __P((void));
static int	twodigits __P((char *, int));

static int	divisor;	/* Systemclock divisor	*/

/*
 * Statistics and profile clock intervals and variances. Variance must
 * be a power of 2. Since this gives us an even number, not an odd number,
 * we discard one case and compensate. That is, a variance of 64 would
 * give us offsets in [0..63]. Instead, we take offsets in [1..63].
 * This is symetric around the point 32, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
#ifdef STATCLOCK
static int	statvar = 32;	/* {stat,prof}clock variance		*/
static int	statmin;	/* statclock divisor - variance/2	*/
static int	profmin;	/* profclock divisor - variance/2	*/
static int	clk2min;	/* current, from above choises		*/
#endif

int
clockmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	if (!atari_realconfig) {
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

	    /*
	     * Initialize the time structure
	     */
	    time.tv_sec  = 0;
	    time.tv_usec = 0;

	    return 0;
	}
	if(!strcmp("clock", auxp))
		return(1);
	return(0);
}

/*
 * Start the real-time clock.
 */
void clockattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct clock_softc *sc = (void *)dp;

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

	if (hz != 48 && hz != 64 && hz != 96) { /* XXX */
		printf (": illegal value %d for systemclock, reset to %d\n\t",
								hz, 64);
		hz = 64;
	}
	printf(": system hz %d timer-A divisor 200/%d\n", hz, divisor);

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

void cpu_initclocks()
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
setstatclockrate(newhz)
	int newhz;
{
#ifdef STATCLOCK
	if (newhz == stathz)
		clk2min = statmin;
	else clk2min = profmin;
#endif /* STATCLOCK */
}

#ifdef STATCLOCK
void
statintr(frame)
	struct clockframe frame;
{
	register int	var, r;

	var = statvar - 1;
	do {
		r = random() & var;
	} while(r == 0);

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

/*
 * Returns number of usec since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
long
clkread()
{
	u_int	delta;

	delta = ((divisor - MFP->mf_tadr) * tick) / divisor;
	/*
	 * Account for pending clock interrupts
	 */
	if(MFP->mf_iera & IA_TIMA)
		return(delta + tick);
	return(delta);
}

#define TIMB_FREQ	614400
#define TIMB_LIMIT	256

/*
 * Wait "n" microseconds.
 * Relies on MFP-Timer B counting down from TIMB_LIMIT at TIMB_FREQ Hz.
 * Note: timer had better have been programmed before this is first used!
 */
void
delay(n)
int	n;
{
	int	tick, otick;

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	otick = MFP->mf_tbdr;

	/*
	 * Calculate ((n * TIMER_FREQ) / 1e6) using explicit assembler code so
	 * we can take advantage of the intermediate 64-bit quantity to prevent
	 * loss of significance.
	 */
	n -= 5;
	if(n < 0)
		return;
	{
	    u_int	temp;
		
	    __asm __volatile ("mulul %2,%1:%0" : "=d" (n), "=d" (temp)
					       : "d" (TIMB_FREQ));
	    __asm __volatile ("divul %1,%2:%0" : "=d" (n)
					       : "d"(1000000),"d"(temp),"0"(n));
	}

	while(n > 0) {
		tick = MFP->mf_tbdr;
		if(tick > otick)
			n -= TIMB_LIMIT - (tick - otick);
		else n -= otick - tick;
		otick = tick;
	}
}

#ifdef GPROF
/*
 * profclock() is expanded in line in lev6intr() unless profiling kernel.
 * Assumes it is called with clock interrupts blocked.
 */
profclock(pc, ps)
	caddr_t pc;
	int ps;
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
			kcount[s / (HISTFRACTION * sizeof (*kcount))]++;
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

u_int mc146818_read(rtc, regno)
void	*rtc;
u_int	regno;
{
	((struct rtc *)rtc)->rtc_regno = regno;
	return(((struct rtc *)rtc)->rtc_data & 0377);
}

void mc146818_write(rtc, regno, value)
void	*rtc;
u_int	regno, value;
{
	((struct rtc *)rtc)->rtc_regno = regno;
	((struct rtc *)rtc)->rtc_data  = value;
}

/*
 * Initialize the time of day register, assuming the RTC runs in UTC.
 * Since we've got the 'rtc' device, this functionality should be removed
 * from the kernel. The only problem to be solved before that can happen
 * is the possibility of init(1) providing a way (rc.boot?) to set
 * the RTC before single-user mode is entered.
 */
void
inittodr(base)
time_t base;
{
	/* Battery clock does not store usec's, so forget about it. */
	time.tv_sec  = gettod();
	time.tv_usec = 0;
}

/*
 * Function turned into a No-op. Use /dev/rtc to update the RTC.
 */
void
resettodr()
{
	return;
}

static u_long
gettod()
{
	int			sps;
	mc_todregs		clkregs;
	struct clock_ymdhms	dt;

	sps = splhigh();
	MC146818_GETTOD(RTC, &clkregs);
	splx(sps);

	if(clkregs[MC_SEC] > 59)
		return(0);
	if(clkregs[MC_MIN] > 59)
		return(0);
	if(clkregs[MC_HOUR] > 23)
		return(0);
	if(range_test(clkregs[MC_DOM], 1, 31))
		return(0);
	if (range_test(clkregs[MC_MONTH], 1, 12))
		return(0);
	if(clkregs[MC_YEAR] > (2000 - GEMSTARTOFTIME))
		return(0);

	dt.dt_year = clkregs[MC_YEAR] + GEMSTARTOFTIME;
	dt.dt_mon  = clkregs[MC_MONTH];
	dt.dt_day  = clkregs[MC_DOM];
	dt.dt_hour = clkregs[MC_HOUR];
	dt.dt_min  = clkregs[MC_MIN];
	dt.dt_sec  = clkregs[MC_SEC];

	return(clock_ymdhms_to_secs(&dt));
}
/***********************************************************************
 *                   RTC-device support				       *
 ***********************************************************************/
int
rtcopen(dev, flag, mode, p)
	dev_t		dev;
	int		flag, mode;
	struct proc	*p;
{
	int			unit = minor(dev);
	struct clock_softc	*sc;

	if (unit >= clock_cd.cd_ndevs)
		return ENXIO;
	sc = clock_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;
	if (sc->sc_flags & RTC_OPEN)
		return EBUSY;

	sc->sc_flags = RTC_OPEN;
	return 0;
}

int
rtcclose(dev, flag, mode, p)
	dev_t		dev;
	int		flag;
	int		mode;
	struct proc	*p;
{
	int			unit = minor(dev);
	struct clock_softc	*sc = clock_cd.cd_devs[unit];

	sc->sc_flags = 0;
	return 0;
}

int
rtcread(dev, uio, flags)
	dev_t		dev;
	struct uio	*uio;
	int		flags;
{
	struct clock_softc	*sc;
	mc_todregs		clkregs;
	int			s, length;
	char			buffer[16];

	sc = clock_cd.cd_devs[minor(dev)];

	s = splhigh();
	MC146818_GETTOD(RTC, &clkregs);
	splx(s);

	sprintf(buffer, "%4d%02d%02d%02d%02d.%02d\n",
	    clkregs[MC_YEAR] + GEMSTARTOFTIME,
	    clkregs[MC_MONTH], clkregs[MC_DOM],
	    clkregs[MC_HOUR], clkregs[MC_MIN], clkregs[MC_SEC]);

	if (uio->uio_offset > strlen(buffer))
		return 0;

	length = strlen(buffer) - uio->uio_offset;
	if (length > uio->uio_resid)
		length = uio->uio_resid;

	return(uiomove((caddr_t)buffer, length, uio));
}

static int
twodigits(buffer, pos)
	char *buffer;
	int pos;
{
	int result = 0;

	if (buffer[pos] >= '0' && buffer[pos] <= '9')
		result = (buffer[pos] - '0') * 10;
	if (buffer[pos+1] >= '0' && buffer[pos+1] <= '9')
		result += (buffer[pos+1] - '0');
	return(result);
}

int
rtcwrite(dev, uio, flags)
	dev_t		dev;
	struct uio	*uio;
	int		flags;
{
	mc_todregs		clkregs;
	int			s, length, error;
	char			buffer[16];

	/*
	 * We require atomic updates!
	 */
	length = uio->uio_resid;
	if (uio->uio_offset || (length != sizeof(buffer)
	  && length != sizeof(buffer - 1)))
		return(EINVAL);
	
	if ((error = uiomove((caddr_t)buffer, sizeof(buffer), uio)))
		return(error);

	if (length == sizeof(buffer) && buffer[sizeof(buffer) - 1] != '\n')
		return(EINVAL);

	s = splclock();
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

	return(0);
}
