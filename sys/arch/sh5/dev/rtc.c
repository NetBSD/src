/*	$NetBSD: rtc.c,v 1.8 2003/07/18 21:41:24 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SH-5 Real Time Clock/Calendar Device
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.8 2003/07/18 21:41:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <sh5/sh5/clockvar.h>

#include <sh5/dev/pbridgevar.h>
#include <sh5/dev/rtcreg.h>
#include <sh5/dev/rtcvar.h>


struct rtc_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	struct todr_chip_handle sc_todr;
};

#define	rtc_read(s,r)    bus_space_read_1((s)->sc_bust, (s)->sc_bush, (r))
#define	rtc_write(s,r,v) bus_space_write_1((s)->sc_bust, (s)->sc_bush, (r), (v))

static int rtcmatch(struct device *, struct cfdata *, void *);
static void rtcattach(struct device *, struct device *, void *);

CFATTACH_DECL(rtc, sizeof(struct rtc_softc),
    rtcmatch, rtcattach, NULL, NULL);
extern struct cfdriver rtc_cd;

static int rtc_gettime(struct todr_chip_handle *, struct timeval *);
static int rtc_settime(struct todr_chip_handle *, struct timeval *);
static int rtc_getcal(struct todr_chip_handle *, int *);
static int rtc_setcal(struct todr_chip_handle *, int);


/*ARGSUSED*/
static int
rtcmatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct pbridge_attach_args *pa = args;

	return (strcmp(pa->pa_name, rtc_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
rtcattach(struct device *parent, struct device *self, void *args)
{
	struct pbridge_attach_args *pa = args;
	struct rtc_softc *sc = (struct rtc_softc *)self;

	sc->sc_bust = pa->pa_bust;
	bus_space_map(sc->sc_bust, pa->pa_offset, RTC_REG_SIZE, 0,&sc->sc_bush);

	bus_space_write_1(sc->sc_bust, sc->sc_bush, RTC_REG_RCR1, 0);
	bus_space_write_1(sc->sc_bust, sc->sc_bush, RTC_REG_RCR2,
	    RTC_RCR2_START|RTC_RCR2_RTCEN);

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = rtc_gettime;
	sc->sc_todr.todr_settime = rtc_settime;
	sc->sc_todr.todr_getcal = rtc_getcal;
	sc->sc_todr.todr_setcal = rtc_setcal;
	sc->sc_todr.todr_setwen = NULL;

	printf(" Real-time Clock\n");

	todr_attach(&sc->sc_todr);
}

static int
rtc_gettime(struct todr_chip_handle *todr, struct timeval *tv)
{
	struct rtc_softc *sc = todr->cookie;
	struct clock_ymdhms dt;
	u_int8_t r64cnt;
	u_int8_t sec, min, hour;
	u_int8_t dow, day, mon;
	u_int16_t year;
	int s, retry;

	retry = 10;
	s = splhigh();

	do {
		r64cnt = rtc_read_r64cnt(sc->sc_bust, sc->sc_bush);

		sec = rtc_read(sc, RTC_REG_RSECCNT);
		min = rtc_read(sc, RTC_REG_RMINCNT);
		hour = rtc_read(sc, RTC_REG_RHRCNT);
		dow = rtc_read(sc, RTC_REG_RWKCNT);
		day = rtc_read(sc, RTC_REG_RDAYCNT);
		mon = rtc_read(sc, RTC_REG_RMONCNT);
		year = bus_space_read_2(sc->sc_bust, sc->sc_bush,
		    RTC_REG_RYRCNT);

		retry--;
	} while (r64cnt != rtc_read_r64cnt(sc->sc_bust, sc->sc_bush) && retry);

	splx(s);

	if (retry == 0) {
		printf("%s: rtc_gettime: WARNING - Failed to read date/time\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	dt.dt_sec = FROMBCD(sec);
	dt.dt_min = FROMBCD(min);
	dt.dt_hour = FROMBCD(hour);
	dt.dt_day = FROMBCD(day);
	dt.dt_wday = FROMBCD(dow);
	dt.dt_mon = FROMBCD(mon);
	dt.dt_year = FROMBCD((year >> 8) & 0xff) * 1000;
	dt.dt_year += FROMBCD(year & 0xff);

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = (1000000 / (RTC_R64CNT_MASK + 1)) * r64cnt;

	return (0);
}

static int
rtc_settime(struct todr_chip_handle *todr, struct timeval *tv)
{
	struct rtc_softc *sc = todr->cookie;
	struct clock_ymdhms dt;
	u_int8_t r64cnt;
	u_int8_t sec, min, hour;
	u_int8_t dow, day, mon;
	u_int16_t year;
	int s, retry;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	sec = TOBCD(dt.dt_sec);
	min = TOBCD(dt.dt_min);
	hour = TOBCD(dt.dt_hour);
	day = TOBCD(dt.dt_day);
	dow = TOBCD(dt.dt_wday);
	mon = TOBCD(dt.dt_mon);
	year = TOBCD(dt.dt_year % 1000);
	year |= TOBCD(dt.dt_year / 1000) << 8;

	retry = 10;
	s = splhigh();

	do {
		r64cnt = rtc_read_r64cnt(sc->sc_bust, sc->sc_bush);

		bus_space_write_2(sc->sc_bust, sc->sc_bush,
		    RTC_REG_RYRCNT, year);
		rtc_write(sc, RTC_REG_RMONCNT, mon);
		rtc_write(sc, RTC_REG_RDAYCNT, day);
		rtc_write(sc, RTC_REG_RWKCNT, dow);
		rtc_write(sc, RTC_REG_RHRCNT, hour);
		rtc_write(sc, RTC_REG_RMINCNT, min);
		rtc_write(sc, RTC_REG_RSECCNT, sec);

		retry--;
	} while (r64cnt != rtc_read_r64cnt(sc->sc_bust, sc->sc_bush) && retry);

	splx(s);

	if (retry == 0) {
		printf("%s: rtc_settime: WARNING - Failed to write date/time\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	return (0);
}

static int
rtc_getcal(struct todr_chip_handle *todr, int *calp)
{

	return (EOPNOTSUPP);
}

static int
rtc_setcal(struct todr_chip_handle *todr, int cal)
{

	return (EOPNOTSUPP);
}

/*
 * Not static; machine-dependent code uses this to
 * calculate the delay() constant.
 */
u_int8_t
rtc_read_r64cnt(bus_space_tag_t bt, bus_space_handle_t bh)
{
	u_int8_t rv;

	do {
		/* Clear Carry Flag */
		bus_space_write_1(bt, bh, RTC_REG_RCR1, 0);
		rv = bus_space_read_1(bt, bh, RTC_REG_R64CNT);
	} while ((bus_space_read_1(bt, bh, RTC_REG_RCR1) & RTC_RCR1_CF) != 0);

	return (rv);
}
