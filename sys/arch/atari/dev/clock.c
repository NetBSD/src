/*	$NetBSD: clock.c,v 1.2 1995/05/05 16:31:46 leo Exp $	*/

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
#include <sys/device.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/dev/clockreg.h>

#if defined(PROF) && defined(PROFTIMER)
#include <sys/PROF.h>
#endif


/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 *
 * A note on the real-time clock:
 * We actually load the clock with CLK_INTERVAL-1 instead of CLK_INTERVAL.
 * This is because the counter decrements to zero after N+1 enabled clock
 * periods where N is the value loaded into the counter.
 */

int	clockmatch __P((struct device *, struct cfdata *, void *));
void	clockattach __P((struct device *, struct device *, void *));

struct cfdriver clockcd = {
	NULL, "clock", (cfmatch_t)clockmatch, clockattach, 
	DV_DULL, sizeof(struct device), NULL, 0
};

static u_long	gettod __P((void));
static int	settod __P((u_long));

static int	divisor;

int
clockmatch(pdp, cfp, auxp)
struct device *pdp;
struct cfdata *cfp;
void *auxp;
{
	if(!strcmp("clock", auxp))
		return(1);
	return(0);
}

/*
 * Start the real-time clock.
 */
void clockattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void			*auxp;
{
	/*
	 * Initialize Timer-A in the ST-MFP. An exact reduce to HZ is not
	 * possible by hardware. We use a divisor of 64 and reduce by software
	 * with a factor of 4. The MFP clock runs at 2457600Hz. Therefore the
	 * timer runs at an effective rate of: 2457600/(64*4) = 9600Hz. The
	 * following expression works for all 'normal' values of hz.
	 */
	divisor       = 9600/hz;
	MFP->mf_tacr  = 0;		/* Stop timer			*/
	MFP->mf_iera &= ~IA_TIMA;	/* Disable timer interrupts	*/
	MFP->mf_tadr  = divisor;	/* Set divisor			*/

	printf(": system hz %d timer-A divisor %d\n", hz, divisor);

	/*
	 * Initialize Timer-B in the ST-MFP. This timer is used by the 'delay'
	 * function below. This time is setup to be continueously counting from 
	 * 255 back to zero at a frequency of 614400Hz.
	 */
	MFP->mf_tbcr  = 0;		/* Stop timer			*/
	MFP->mf_iera &= ~IA_TIMB;	/* Disable timer interrupts	*/
	MFP->mf_tbdr  = 0;	
	MFP->mf_tbcr  = T_Q004;	/* Start timer			*/
	
}

void cpu_initclocks()
{
	MFP->mf_tacr  = T_Q064;		/* Start timer			*/
	MFP->mf_ipra &= ~IA_TIMA;	/* Clear pending interrupts	*/
	MFP->mf_iera |= IA_TIMA;	/* Enable timer interrupts	*/
	MFP->mf_imra |= IA_TIMA;	/*    .....			*/
}

setstatclockrate(hz)
	int hz;
{
}

/*
 * Returns number of usec since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
clkread()
{
	extern	short	clk_div;
			u_int	delta, elapsed;
   
	elapsed = (divisor - MFP->mf_tadr) + ((4 - clk_div) * divisor);
	delta   = (elapsed * tick) / (divisor << 2);
	
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
void delay(n)
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

#ifdef PROFTIMER
/*
 * This code allows the amiga kernel to use one of the extra timers on
 * the clock chip for profiling, instead of the regular system timer.
 * The advantage of this is that the profiling timer can be turned up to
 * a higher interrupt rate, giving finer resolution timing. The profclock
 * routine is called from the lev6intr in locore, and is a specialized
 * routine that calls addupc. The overhead then is far less than if
 * hardclock/softclock was called. Further, the context switch code in
 * locore has been changed to turn the profile clock on/off when switching
 * into/out of a process that is profiling (startprofclock/stopprofclock).
 * This reduces the impact of the profiling clock on other users, and might
 * possibly increase the accuracy of the profiling. 
 */
int  profint   = PRF_INTERVAL;	/* Clock ticks between interrupts */
int  profscale = 0;		/* Scale factor from sys clock to prof clock */
char profon    = 0;		/* Is profiling clock on? */

/* profon values - do not change, locore.s assumes these values */
#define PRF_NONE	0x00
#define	PRF_USER	0x01
#define	PRF_KERNEL	0x80

initprofclock()
{
#if NCLOCK > 0
	struct proc *p = curproc;		/* XXX */

	/*
	 * If the high-res timer is running, force profiling off.
	 * Unfortunately, this gets reflected back to the user not as
	 * an error but as a lack of results.
	 */
	if (clockon) {
		p->p_stats->p_prof.pr_scale = 0;
		return;
	}
	/*
	 * Keep track of the number of user processes that are profiling
	 * by checking the scale value.
	 *
	 * XXX: this all assumes that the profiling code is well behaved;
	 * i.e. profil() is called once per process with pcscale non-zero
	 * to turn it on, and once with pcscale zero to turn it off.
	 * Also assumes you don't do any forks or execs.  Oh well, there
	 * is always adb...
	 */
	if (p->p_stats->p_prof.pr_scale)
		profprocs++;
	else
		profprocs--;
#endif
	/*
	 * The profile interrupt interval must be an even divisor
	 * of the CLK_INTERVAL so that scaling from a system clock
	 * tick to a profile clock tick is possible using integer math.
	 */
	if (profint > CLK_INTERVAL || (CLK_INTERVAL % profint) != 0)
		profint = CLK_INTERVAL;
	profscale = CLK_INTERVAL / profint;
}

startprofclock()
{
  unsigned short interval;

  /* stop timer B */
  ciab.crb = ciab.crb & 0xc0;

  /* load interval into registers.
     the clocks run at NTSC: 715.909kHz or PAL: 709.379kHz */

  interval = profint - 1;

  /* order of setting is important ! */
  ciab.tblo = interval & 0xff;
  ciab.tbhi = interval >> 8;

  /* enable interrupts for timer B */
  ciab.icr = (1<<7) | (1<<1);

  /* start timer B in continuous shot mode */
  ciab.crb = (ciab.crb & 0xc0) | 1;
}

stopprofclock()
{
  /* stop timer B */
  ciab.crb = ciab.crb & 0xc0;
}

#ifdef PROF
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
#endif

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
inittodr(base)
time_t base;
{
	u_long timbuf = base;	/* assume no battery clock exists */
  
	timbuf = gettod();
  
	if(timbuf < base) {
		printf("WARNING: bad date in battery clock\n");
		timbuf = base;
	}
  
	/* Battery clock does not store usec's, so forget about it. */
	time.tv_sec = timbuf;
}

resettodr()
{
	if(settod(time.tv_sec) == 1)
		return;
	printf("Cannot set battery backed clock\n");
}

static	char	dmsize[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static	char	ldmsize[12] =
{
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static __inline__ int rtc_getclkreg(regno)
int	regno;
{
	RTC->rtc_regno = RTC_REGA;
	RTC->rtc_regno = regno;
	return(RTC->rtc_data & 0377);
}

static __inline__ void rtc_setclkreg(regno, value)
int	regno, value;
{
	RTC->rtc_regno = regno;
	RTC->rtc_data  = value;
}

static u_long
gettod()
{
	int	i, year, mon, day, hour, min, sec;
	u_long	new_time = 0;
	char	*msize;

	/*
	 * Hold clock
	 */
	rtc_setclkreg(RTC_REGB, rtc_getclkreg(RTC_REGB) | RTC_B_SET);

	/*
	 * Read clock
	 */
	sec  = rtc_getclkreg(RTC_SEC);
	min  = rtc_getclkreg(RTC_MIN);
	hour = rtc_getclkreg(RTC_HOUR);
	day  = rtc_getclkreg(RTC_DAY) - 1;
	mon  = rtc_getclkreg(RTC_MONTH) - 1;
	year = rtc_getclkreg(RTC_YEAR) + STARTOFTIME;

	/*
	 * Let it run again..
	 */
	rtc_setclkreg(RTC_REGB, rtc_getclkreg(RTC_REGB) & ~RTC_B_SET);

	if(range_test(hour, 0, 23))
		return(0);
	if(range_test(day, 0, 30))
		return(0);
	if (range_test(mon, 0, 11))
		return(0);
	if(range_test(year, STARTOFTIME, 2000))
		return(0);

	for(i = STARTOFTIME; i < year; i++) {
		if(is_leap(i))
			new_time += 366;
		else new_time += 365;
	}

	msize = is_leap(year) ? ldmsize : dmsize;
	for(i = 0; i < mon; i++)
		new_time += msize[i];
	new_time += day;
	return((new_time * SECS_DAY) + (hour * 3600) + (min * 60) + sec);
}

static int
settod(newtime)
u_long	newtime;
{
	register long	days, rem, year;
	register char	*ml;
			 int	sec, min, hour, month;

	/* Number of days since Jan. 1 1970	*/
	days = newtime / SECS_DAY;
	rem  = newtime % SECS_DAY;

	/*
	 * Calculate sec, min, hour
	 */
	hour = rem / SECS_HOUR;
	rem %= SECS_HOUR;
	min  = rem / 60;
	sec  = rem % 60;

	/*
	 * Figure out the year. Day in year is left in 'days'.
	 */
	year = STARTOFTIME;
	while(days >= (rem = is_leap(year) ? 366 : 365)) {
	  ++year;
	  days -= rem;
	}
	while(days < 0) {
	  --year;
	  days += is_leap(year) ? 366 : 365;
	}

	/*
	 * Determine the month
	 */
	ml = is_leap(year) ? ldmsize : dmsize;
	for(month = 0; days >= ml[month]; ++month)
		days -= ml[month];

	/*
	 * Now that everything is calculated, program the RTC
	 */
	rtc_setclkreg(RTC_REGB, RTC_B_SET);
	rtc_setclkreg(RTC_REGA, RTC_A_DV1|RTC_A_RS2|RTC_A_RS3);
	rtc_setclkreg(RTC_REGB, RTC_B_SET|RTC_B_SQWE|RTC_B_DM|RTC_B_24_12);
	rtc_setclkreg(RTC_SEC, sec);
	rtc_setclkreg(RTC_MIN, min);
	rtc_setclkreg(RTC_HOUR, hour);
	rtc_setclkreg(RTC_DAY, days+1);
	rtc_setclkreg(RTC_MONTH, month+1);
	rtc_setclkreg(RTC_YEAR, year-1970);
	rtc_setclkreg(RTC_REGB, RTC_B_SQWE|RTC_B_DM|RTC_B_24_12);

	return(1);
}
