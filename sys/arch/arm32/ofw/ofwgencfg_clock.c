/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/* Include header files */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>

#include <machine/irqhandler.h>
#include <machine/cpu.h>
#include <machine/ofw.h>

/*
static irqhandler_t clockirq;
static irqhandler_t statclockirq;
*/
static void *clockirq;
static void *statclockirq;


/*
 * int clockhandler(struct clockframe *frame)
 *
 * Function called by timer 0 interrupts. This just calls
 * hardclock(). Eventually the irqhandler can call hardclock() directly
 * but for now we use this function so that we can debug IRQ's
 */
 
int
clockhandler(frame)
	struct clockframe *frame;
{
	hardclock(frame);
	return(0);	/* Pass the interrupt on down the chain */
}


/*
 * int statclockhandler(struct clockframe *frame)
 *
 * Function called by timer 1 interrupts. This just calls
 * statclock(). Eventually the irqhandler can call statclock() directly
 * but for now we use this function so that we can debug IRQ's
 */
 
int
statclockhandler(frame)
	struct clockframe *frame;
{
	statclock(frame);
	return(0);	/* Pass the interrupt on down the chain */
}


/*
 * void setstatclockrate(int hz)
 *
 * Set the stat clock rate. The stat clock uses timer1
 */

void
setstatclockrate(hz)
	int hz;
{
	int count;
    
#ifdef	OFWGENCFG
	printf("Not setting statclock: OFW generic has only one clock.\n");
#endif
}


/*
 * void cpu_initclocks(void)
 *
 * Initialise the clocks.
 * This sets up the two timers in the IOMD and installs the IRQ handlers
 *
 * NOTE: Currently only timer 0 is setup and the IRQ handler is not installed
 */
 
void
cpu_initclocks()
{
	/*
	 * Load timer 0 with count down value
	 * This timer generates 100Hz interrupts for the system clock
	 */

	printf("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

/*
	clockirq.ih_func = clockhandler;
	clockirq.ih_arg = 0;
	clockirq.ih_level = IPL_CLOCK;
	clockirq.ih_name = "TMR0 hard clk";
	if (irq_claim(IRQ_TIMER0, &clockirq) == -1)
		panic("Cannot installer timer 0 IRQ handler\n");
*/
        clockirq = intr_claim(IRQ_TIMER0, IPL_CLOCK, "tmr0 hard clk",
            clockhandler, 0);
        if (clockirq == NULL)
                panic("Cannot installer timer 0 IRQ handler\n");

	/* Notify callback handler that it can start processing ticks. */
	ofw_handleticks = 1;

	if (stathz) {
	    printf("Not installing statclock: OFW generic has only one clock.\n");
	}
}


/*
 * void microtime(struct timeval *tvp)
 *
 * Fill in the specified timeval struct with the current time
 * accurate to the microsecond.
 */

void
microtime(tvp)
	struct timeval *tvp;
{
	int s;
	static struct timeval oldtv;

	s = splhigh();

	/* Fill in the timeval struct */

	*tvp = time;    

	/* Make sure the micro seconds don't overflow. */

	while (tvp->tv_usec > 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}

	/* Make sure the time has advanced. */

	if (tvp->tv_sec == oldtv.tv_sec &&
	    tvp->tv_usec <= oldtv.tv_usec) {
		tvp->tv_usec = oldtv.tv_usec + 1;
		if (tvp->tv_usec > 1000000) {
			tvp->tv_usec -= 1000000;
			++tvp->tv_sec;
		}
	}
	    

	oldtv = *tvp;
	(void)splx(s);		
}


void
need_proftick(p)
	struct proc *p;
{
}


/*
 *  Time-of-day clock
 *
 *  Cribbed from dev/todclock.c
 *  Need to integrate with that code!
 */

#include <machine/rtc.h>

static rtc_t fake_rtc =	{0, 0, 0, 0, 0,	11, 3, 97, 19};	/* March 11, 1997, 00:00:00 */

static int
fake_rtc_write(rtc)
    rtc_t *rtc;
{
    fake_rtc = *rtc;
    return(1);
}


static int
fake_rtc_read(rtc)
    rtc_t *rtc;
{
    *rtc = fake_rtc;
    return(1);
}


static __inline int
yeartoday(year)
	int year;
{
	return((year % 4) ? 365 : 366);
}

                 
static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int timeset = 0;

#define SECPERDAY	(24*60*60)
#define SECPERNYEAR	(365*SECPERDAY)
#define SECPER4YEARS	(4*SECPERNYEAR+SECPERDAY)
#define EPOCHYEAR	1970

/*
 * Globally visable functions
 *
 * These functions are used from other parts of the kernel.
 * These functions use the functions defined in the tod_sc
 * to actually read and write the rtc.
 *
 * The first todclock to be attached will be used for handling
 * the time of day.
 */

/*
 * Write back the time of day to the rtc
 */

void
resettodr()
{
	int s;
	time_t year, mon, day, hour, min, sec;
	rtc_t rtc;

	/* Have we set the system time in inittodr() */
	if (!timeset)
		return;

	sec = time.tv_sec;
	sec -= rtc_offset * 60;
	year = (sec / SECPER4YEARS) * 4;
	sec %= SECPER4YEARS;

	/* year now hold the number of years rounded down 4 */

	while (sec > (yeartoday(EPOCHYEAR+year) * SECPERDAY)) {
		sec -= yeartoday(EPOCHYEAR+year)*SECPERDAY;
		year++;
	}

	/* year is now a correct offset from the EPOCHYEAR */

	year+=EPOCHYEAR;
	mon=0;
	if (yeartoday(year) == 366)
		month[1]=29;
	else
		month[1]=28;
	while ((sec/SECPERDAY) > month[mon]) {
		sec -= month[mon]*SECPERDAY;
		mon++;
	}

	day = sec / SECPERDAY;
	sec %= SECPERDAY;
	hour = sec / 3600;
	sec %= 3600;
	min = sec / 60;
	sec %= 60;
	rtc.rtc_cen = year / 100;
	rtc.rtc_year = year % 100;
	rtc.rtc_mon = mon+1;
	rtc.rtc_day = day+1;
	rtc.rtc_hour = hour;
	rtc.rtc_min = min;
	rtc.rtc_sec = sec;
	rtc.rtc_centi =
	rtc.rtc_micro = 0;

/*
	printf("resettod: %d/%d/%d%d %d:%d:%d\n", rtc.rtc_day,
	    rtc.rtc_mon, rtc.rtc_cen, rtc.rtc_year, rtc.rtc_hour,
	    rtc.rtc_min, rtc.rtc_sec);
*/

	s = splclock();

/*
	if (!todclock_sc)
		panic("resettod: No todclock device attached\n");
	todclock_sc->sc_rtc_write(&rtc);
*/
	fake_rtc_write(&rtc);
	(void)splx(s);
}

/*
 * Initialise the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */

void
inittodr(base)
	time_t base;
{
	time_t n;
	int i, days = 0;
	int s;
	int year;
	rtc_t rtc;

	/*
	 * We ignore the suggested time for now and go for the RTC
	 * clock time stored in the CMOS RAM.
	 */

/*
	if (!todclock_sc)
		panic("resettod: No todclock device attached\n");
*/

	s = splclock();
/*
	if (todclock_sc->sc_rtc_read(&rtc)  == 0) {
		(void)splx(s);
		return;
	}
*/
	(void)fake_rtc_read(&rtc);

	(void)splx(s);

	n = rtc.rtc_sec + 60 * rtc.rtc_min + 3600 * rtc.rtc_hour;
	n += (rtc.rtc_day - 1) * 3600 * 24;
	year = (rtc.rtc_year + rtc.rtc_cen * 100) - 1900;

	if (yeartoday(year) == 366)
		month[1] = 29;
	for (i = rtc.rtc_mon - 2; i >= 0; i--)
		days += month[i];
	month[1] = 28;

	for (i = 70; i < year; i++)
		days += yeartoday(i);

	n += days * 3600 * 24;

	n += rtc_offset * 60;

	time.tv_sec = n;
	time.tv_usec = 0;

	/* timeset is used to ensure the time is valid before a resettodr() */

	timeset = 1;

	/* If the base was 0 then keep quiet */

	if (base) {
		printf("inittodr: %02d:%02d:%02d.%02d%02d %02d/%02d/%02d%02d\n",
		    rtc.rtc_hour, rtc.rtc_min, rtc.rtc_sec, rtc.rtc_centi,
		    rtc.rtc_micro, rtc.rtc_day, rtc.rtc_mon, rtc.rtc_cen,
		    rtc.rtc_year);

		if (n > base + 60) {
			days = (n - base) / SECPERDAY;
			printf("Clock has gained %d day%c %ld hours %ld minutes %ld secs\n",
			    days, ((days == 1) ? 0 : 's'), ((n - base)  / 3600) % 24,
			    ((n - base) / 60) % 60, (n - base) % 60);
		}
	}
}  
