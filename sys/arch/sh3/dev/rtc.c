/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.2 2007/03/30 00:32:58 uwe Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <dev/clock_subr.h>

#include <sh3/rtcreg.h>

#if defined(DEBUG) && !defined(RTC_DEBUG)
#define RTC_DEBUG
#endif


struct rtc_softc {
	struct device sc_dev;

	int sc_valid;
	struct todr_chip_handle sc_todr;
};

static int	rtc_match(struct device *, struct cfdata *, void *);
static void	rtc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(rtc, sizeof(struct rtc_softc),
    rtc_match, rtc_attach, NULL, NULL);


/* todr(9) methods */
static int rtc_gettime_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
static int rtc_settime_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);



static int
rtc_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	return 1;
}


static void
rtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct rtc_softc *sc = (void *)self;
	uint8_t r;
#ifdef RTC_DEBUG
	char bits[128];
#endif

	printf("\n");

	r = _reg_read_1(SH_(RCR2));

#ifdef RTC_DEBUG
	printf("%s: RCR2=%s\n", sc->sc_dev.dv_xname,
	       bitmask_snprintf(r, SH_RCR2_BITS, bits, sizeof(bits)));
#endif

	/* Was the clock running? */
	if ((r & (SH_RCR2_ENABLE | SH_RCR2_START)) == (SH_RCR2_ENABLE
						       | SH_RCR2_START))
		sc->sc_valid = 1;
	else {
		sc->sc_valid = 0;
		printf("%s: WARNING: clock was stopped\n",
		       sc->sc_dev.dv_xname);
	}

	/* Disable carry and alarm interrupts */
	_reg_write_1(SH_(RCR1), 0);

	/* Clock runs, no periodic interrupts, no 30-sec adjustment */
	_reg_write_1(SH_(RCR2), SH_RCR2_ENABLE | SH_RCR2_START);

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = rtc_gettime_ymdhms;
	sc->sc_todr.todr_settime_ymdhms = rtc_settime_ymdhms;

	todr_attach(&sc->sc_todr);

#ifdef RTC_DEBUG
	{
		struct clock_ymdhms dt;
		rtc_gettime_ymdhms(&sc->sc_todr, &dt);
	}
#endif
}


static int
rtc_gettime_ymdhms(todr_chip_handle_t h, struct clock_ymdhms *dt)
{
	struct rtc_softc *sc = h->cookie;
	unsigned int year;
	int retry = 8;

	if (!sc->sc_valid) {
#ifdef RTC_DEBUG
		printf("RTC: gettime: not valid\n");
		/* but proceed and read/print it anyway */
#else
		return EIO;
#endif
	}

	/* disable carry interrupt */
	_reg_bclr_1(SH_(RCR1), SH_RCR1_CIE);

	do {
		uint8_t r = _reg_read_1(SH_(RCR1));
		r &= ~SH_RCR1_CF;
		r |= SH_RCR1_AF; /* don't clear alarm flag */
		_reg_write_1(SH_(RCR1), r);

		if (CPU_IS_SH3)
			year = _reg_read_1(SH3_RYRCNT);
		else
			year = _reg_read_2(SH4_RYRCNT) & 0x00ff;
		dt->dt_year = FROMBCD(year);

		/* read counter */
#define	RTCGET(x, y) \
		dt->dt_ ## x = FROMBCD(_reg_read_1(SH_(R ## y ## CNT)))

		RTCGET(mon, MON);
		RTCGET(wday, WK);
		RTCGET(day, DAY);
		RTCGET(hour, HR);
		RTCGET(min, MIN);
		RTCGET(sec, SEC);
#undef RTCGET
	} while ((_reg_read_1(SH_(RCR1)) & SH_RCR1_CF) && --retry > 0);

	if (retry == 0) {
#ifdef RTC_DEBUG
		printf("RTC: gettime: retry failed\n");
#endif
		return EIO;
	}

	dt->dt_year += 1900;
	if (dt->dt_year < POSIX_BASE_YEAR)
		dt->dt_year += 100;

#ifdef RTC_DEBUG
	printf("RTC: gettime: %04d-%02d-%02d %02d:%02d:%02d\n",
	       dt->dt_year, dt->dt_mon, dt->dt_day,
	       dt->dt_hour, dt->dt_min, dt->dt_sec);

	if (!sc->sc_valid)
		return EIO;
#endif

	return 0;
}


static int
rtc_settime_ymdhms(todr_chip_handle_t h, struct clock_ymdhms *dt)
{
	struct rtc_softc *sc = h->cookie;
	unsigned int year;
	uint8_t r;

	year = TOBCD(dt->dt_year % 100);

	r = _reg_read_1(SH_(RCR2));

	/* stop clock */
	_reg_write_1(SH_(RCR2), (r & ~SH_RCR2_START) | SH_RCR2_RESET);

	/* set time */
	if (CPU_IS_SH3)
		_reg_write_1(SH3_RYRCNT, year);
	else
		_reg_write_2(SH4_RYRCNT, year);

#define	RTCSET(x, y) \
	_reg_write_1(SH_(R ## x ## CNT), TOBCD(dt->dt_ ## y))

	RTCSET(MON, mon);
	RTCSET(WK, wday);
	RTCSET(DAY, day);
	RTCSET(HR, hour);
	RTCSET(MIN, min);
	RTCSET(SEC, sec);

#undef RTCSET

	/* start clock */
	_reg_write_1(SH_(RCR2), r);
	sc->sc_valid = 1;

#ifdef RTC_DEBUG
	printf("RTC: settime: %04d-%02d-%02d %02d:%02d:%02d\n",
	       dt->dt_year, dt->dt_mon, dt->dt_day,
	       dt->dt_hour, dt->dt_min, dt->dt_sec);
#endif

	return 0;
}
