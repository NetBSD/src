/*	$NetBSD: rtclock.c,v 1.10 2001/05/26 21:32:30 minoura Exp $	*/

/*
 * Copyright 1993, 1994 Masaru Oki
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
 *      This product includes software developed by Masaru Oki.
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

/*
 * X680x0 internal real time clock interface
 * alarm is not supported.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/clock_subr.h>

#include <arch/x68k/dev/rtclock_var.h>
#include <arch/x68k/dev/intiovar.h>

static time_t rtgettod __P((void));
static int  rtsettod __P((long));

static int rtc_match __P((struct device *, struct cfdata *, void *));
static void rtc_attach __P((struct device *, struct device *, void *));

int rtclockinit __P((void));

struct cfattach rtc_ca = {
	sizeof(struct rtc_softc), rtc_match, rtc_attach
};

static int
rtc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct intio_attach_args *ia = aux;

	if (strcmp (ia->ia_name, "rtc") != 0)
		return (0);
	if (cf->cf_unit != 0)
		return (0);

	/* fixed address */
	if (ia->ia_addr != RTC_ADDR)
		return (0);
	if (ia->ia_intr != -1)
		return (0);

	return (1);
}


static struct rtc_softc *rtc;	/* XXX: softc cache */

static void
rtc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rtc_softc *sc = (struct rtc_softc *)self;
	struct intio_attach_args *ia = aux;
	int r;

	ia->ia_size = 0x20;
	r = intio_map_allocate_region (parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic ("IO map for RTC corruption??");
#endif


	sc->sc_bst = ia->ia_bst;
	bus_space_map(sc->sc_bst, ia->ia_addr, 0x2000, 0, &sc->sc_bht);
	rtc = sc;

	rtclockinit();
	printf (": RP5C15\n");
}



/*
 * x68k/clock.c calls thru this vector, if it is set, to read
 * the realtime clock.
 */
time_t (*gettod) __P((void));
int (*settod) __P((long));

int
rtclockinit()
{
	if (rtgettod())	{
		gettod = rtgettod;
		settod = rtsettod;
	} else {
		return 0;
	}
	return 1;
}

static time_t
rtgettod()
{
	struct clock_ymdhms dt;

	/* hold clock */
	RTC_WRITE(RTC_MODE, RTC_HOLD_CLOCK);

	/* read it */
	dt.dt_sec  = RTC_REG(RTC_SEC10)  * 10 + RTC_REG(RTC_SEC);
	dt.dt_min  = RTC_REG(RTC_MIN10)  * 10 + RTC_REG(RTC_MIN);
	dt.dt_hour = RTC_REG(RTC_HOUR10) * 10 + RTC_REG(RTC_HOUR);
	dt.dt_day  = RTC_REG(RTC_DAY10)  * 10 + RTC_REG(RTC_DAY);
	dt.dt_mon  = RTC_REG(RTC_MON10)  * 10 + RTC_REG(RTC_MON);
	dt.dt_year = RTC_REG(RTC_YEAR10) * 10 + RTC_REG(RTC_YEAR)
							+RTC_BASE_YEAR;

	/* let it run again.. */
	RTC_WRITE(RTC_MODE, RTC_FREE_CLOCK);

#ifdef DIAGNOSTIC
	range_test0(dt.dt_hour, 23);
	range_test(dt.dt_day, 1, 31);
	range_test(dt.dt_mon, 1, 12);
	range_test(dt.dt_year, RTC_BASE_YEAR, RTC_BASE_YEAR+100-1);
#endif
  
	return clock_ymdhms_to_secs (&dt) + rtc_offset * 60;
}

static int
rtsettod (tim)
	time_t tim;
{
	struct clock_ymdhms dt;
	u_char sec1, sec2;
	u_char min1, min2;
	u_char hour1, hour2;
	u_char day1, day2;
	u_char mon1, mon2;
	u_char year1, year2;

	clock_secs_to_ymdhms (tim - rtc_offset * 60, &dt);

	/* prepare values to be written to clock */
	sec1  = dt.dt_sec  / 10;
	sec2  = dt.dt_sec  % 10;
	min1  = dt.dt_min  / 10;
	min2  = dt.dt_min  % 10;
	hour1 = dt.dt_hour / 10;
	hour2 = dt.dt_hour % 10;

	day1  = dt.dt_day  / 10;
	day2  = dt.dt_day  % 10;
	mon1  = dt.dt_mon  / 10;
	mon2  = dt.dt_mon  % 10;
	year1 = (dt.dt_year - RTC_BASE_YEAR) / 10;
	year2 = dt.dt_year % 10;

	RTC_WRITE(RTC_MODE,   RTC_HOLD_CLOCK);
	RTC_WRITE(RTC_SEC10,  sec1);
	RTC_WRITE(RTC_SEC,    sec2);
	RTC_WRITE(RTC_MIN10,  min1);
	RTC_WRITE(RTC_MIN,    min2);
	RTC_WRITE(RTC_HOUR10, hour1);
	RTC_WRITE(RTC_HOUR,   hour2);
	RTC_WRITE(RTC_DAY10,  day1);
	RTC_WRITE(RTC_DAY,    day2);
	RTC_WRITE(RTC_MON10,  mon1);
	RTC_WRITE(RTC_MON,    mon2);
	RTC_WRITE(RTC_YEAR10, year1);
	RTC_WRITE(RTC_YEAR,   year2);
	RTC_WRITE(RTC_MODE,   RTC_FREE_CLOCK);

	return 1;
}
