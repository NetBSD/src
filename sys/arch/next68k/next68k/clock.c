/*	$NetBSD: clock.c,v 1.2 1999/01/27 11:27:17 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/tty.h>

#include <machine/psl.h>
#include <machine/cpu.h>

#include <next68k/dev/clockreg.h>

/* @@@ This is pretty bogus and will need fixing once
 * things are working better.
 * -- jewell@mit.edu
 */

/*
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz) on 68020
 * and 68030 systems.  See clock.c for the delay
 * calibration algorithm.
 */
int	cpuspeed;		  /* relative cpu speed; XXX skewed on 68040 */
int	delay_divisor = 2048/25;  /* delay constant */

/*
 * Calibrate the delay constant.
 */
void
next68k_calibrate_delay()
{
  extern int delay_divisor;

  /* @@@ write this once we know how to read
   * a real time clock
   */

  /*
   * Sanity check the delay_divisor value.  If we totally lost,
   * assume a 25MHz CPU;
   */
  if (delay_divisor == 0)
    delay_divisor = 2048 / 25;

  /* Calculate CPU speed. */
  cpuspeed = 2048 / delay_divisor;
}

#define	SECDAY		(24 * 60 * 60)
#define	SECYR		(SECDAY * 365)

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(base)
        time_t base;
{
  int badbase = 0;

  if (base < 5*SECYR) {
    printf("WARNING: preposterous time in file system");
    base = 6*SECYR + 186*SECDAY + SECDAY/2;
    badbase = 1;
  }

  if ((time.tv_sec = getsecs()) == 0) {
    printf("WARNING: bad date in battery clock");
    /*
     * Believe the time in the file system for lack of
     * anything better, resetting the clock.
     */
    time.tv_sec = base;
    if (!badbase)
      resettodr();
  } else {
    int deltat = time.tv_sec - base;
    
    if (deltat < 0)
      deltat = -deltat;
    if (deltat < 2 * SECDAY)
      return;
    printf("WARNING: clock %s %d days\n",
           time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
  }
}

void
resettodr()
{
  setsecs(time.tv_sec);
}

int clock_intr __P((void *));

int
clock_intr(arg)
     void *arg;
{
  if (!INTR_OCCURRED(NEXT_I_TIMER)) return(0);

	{
		volatile struct timer_reg *timer = IIOV(NEXT_P_TIMER);
		timer->csr |= TIMER_UPDATE;
	}

	hardclock(arg);

  return(1);
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only
 * if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks()
{
  rtc_init();

  hz = 100;

  {
    int s;
    s = splclock();

    {
      volatile struct timer_reg *timer = IIOV(NEXT_P_TIMER);
      int cnt = 1000000/hz;          /* usec timer */
      timer->csr = 0;
      timer->msb = (cnt>>8);
      timer->lsb = cnt;
      timer->csr = TIMER_ENABLE|TIMER_UPDATE;
    }

    isrlink_autovec(clock_intr, NULL, NEXT_I_IPL(NEXT_I_TIMER), 0);
    INTR_ENABLE(NEXT_I_TIMER);

    splx(s);
  }
}


void
setstatclockrate(newhz)
	int newhz;
{

	/* XXX should we do something here? XXX */
}

/* @@@ update this to use the usec timer 
 * Darrin B Jewell <jewell@mit.edu>  Sun Feb  8 05:01:02 1998
 */


/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */

void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec;
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}
