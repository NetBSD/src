/*	$NetBSD: clock.c,v 1.5 1999/12/05 15:50:46 tsubai Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, Ralph Campbell, and Kazumasa Utashiro of
 * Software Research Associates, Inc.
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
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <machine/autoconf.h>
#include <machine/adrsmap.h>

#include <newsmips/newsmips/clockreg.h>

static int clockmatch __P((struct device *, struct cfdata *, void *));
static void clockattach __P((struct device *, struct device *, void *));
void setstatclockrate __P((int));
void cpu_initclocks __P((void));
void inittodr __P((time_t));
void resettodr __P((void));

struct cfattach mkclock_ca = {
	sizeof(struct device), clockmatch, clockattach
};

int
clockmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "mkclock") != 0)
		return 0;

	return 1;
}

void
clockattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	printf("\n");
}

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardare clock lost state.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(newhz)
	int newhz;
{

	/* KU:XXX do something! */
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only if
 * no alternative timer is available.
 */
void
cpu_initclocks()
{

	/*
	 * Start the real-time clock.
	 */
	*(char *)ITIMER = IOCLOCK / 6144 / 100 - 1;

	/*
	 * Enable the real-time clock.
	 */
	*(char *)INTEN0 |= (char)INTEN0_TIMINT;
}

#define	bcd_to_int(BCD)	(i = BCD, (((i) >> 4) & 0xf) * 10 + ((i) & 0xf))
#define	int_to_bcd(INT)	(i = INT, ((((i) / 10) % 10) << 4) + (i) % 10)

/*
 * Initialze the time of day register, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock (if any) provides the rest.
 */
void
inittodr(base)
	time_t base;
{
	register volatile u_char *rtc_port = (u_char *)RTC_PORT;
	register volatile u_char *rtc_data = (u_char *)DATA_PORT;
	int sec, min, hour, week, day, mon, year;
	int deltat, badbase = 0;
	register u_int i;
	struct clock_ymdhms dt;

	if (base < 5*SECYR) {
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = 6*SECYR + 186*SECDAY + SECDAY/2;
		badbase = 1;
	}

	*rtc_port = READ_CLOCK;
	sec  = bcd_to_int(*rtc_data++);
	min  = bcd_to_int(*rtc_data++);
	hour = bcd_to_int(*rtc_data++);
	week = bcd_to_int(*rtc_data++);
	day  = bcd_to_int(*rtc_data++);
	mon  = bcd_to_int(*rtc_data++);
	year = bcd_to_int(*rtc_data++);
	*rtc_port = 0;

	/* simple sanity checks */
	if (mon < 1 || mon > 12 || day < 1 || day > 31 ||
	    hour > 23 || min > 59 || sec > 59) {
		printf("WARNING: preposterous clock chip time\n");
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		time.tv_sec = base;
		if (!badbase)
			resettodr();
		return;
	}

	dt.dt_year = year + (year >= 70 ? 1900 : 2000);
	dt.dt_mon = mon;
	dt.dt_day = day;
	dt.dt_hour = hour;
	dt.dt_min = min;
	dt.dt_sec = sec;

	time.tv_sec = clock_ymdhms_to_secs(&dt);

	if (!badbase) {
		/*
		 * See if we gained/lost two or more days;
		 * if so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;
		printf("WARNING: clock %s %ld days",
		    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
void
resettodr()
{
	register volatile u_char *rtc_port = (u_char *)RTC_PORT;
	register volatile u_char *rtc_data = (u_char *)DATA_PORT;
	register int i;
	struct clock_ymdhms dt;

	clock_secs_to_ymdhms(time.tv_sec, &dt);

	*rtc_port = SET_CLOCK;
	*rtc_data++ = int_to_bcd(dt.dt_sec);
	*rtc_data++ = int_to_bcd(dt.dt_min);
	*rtc_data++ = int_to_bcd(dt.dt_hour);
	*rtc_data++ = int_to_bcd(dt.dt_wday);
	*rtc_data++ = int_to_bcd(dt.dt_day);
	*rtc_data++ = int_to_bcd(dt.dt_mon);
	*rtc_data   = int_to_bcd(dt.dt_year);
	*rtc_port = 0;
}
