/*	$NetBSD: mvsocrtc.c,v 1.2.12.1 2017/12/03 11:35:54 jdolecek Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

/*
 * Driver for real time clock unit on Marvell kirkwood and possibly other
 * SoCs.  Supports only the time/date keeping functions.  No support for
 * the rtc unit alarm / interrupt functions.
 *
 * Written 10/2010 by Brett Slager -- all rights assigned to the NetBSD
 * Foundation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mvsocrtc.c,v 1.2.12.1 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/clock_subr.h>

#include <sys/bus.h>

#include <arm/marvell/mvsocrtcreg.h>
#include <dev/marvell/marvellvar.h>

struct mvsocrtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct todr_chip_handle	sc_todr;
};

static int mvsocrtc_match(device_t, cfdata_t, void *);
static void mvsocrtc_attach(device_t, device_t, void *);
static int mvsocrtc_todr_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int mvsocrtc_todr_settime(todr_chip_handle_t, struct clock_ymdhms *);

CFATTACH_DECL_NEW(mvsocrtc, sizeof(struct mvsocrtc_softc), mvsocrtc_match,
    mvsocrtc_attach, NULL, NULL);

static int
mvsocrtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct marvell_attach_args * const mva = aux;

	if (strcmp(mva->mva_name, cf->cf_name) != 0)
		return 0;
	if (mva->mva_offset == MVA_OFFSET_DEFAULT)
		return 0;

	mva->mva_size = MVSOCRTC_SIZE;
	return 1;
}

static void
mvsocrtc_attach(device_t parent, device_t self, void *aux)
{
	struct mvsocrtc_softc * const sc = device_private(self);
	struct marvell_attach_args * const mva = aux;

	sc->sc_dev = self;
	sc->sc_iot = mva->mva_iot;

	aprint_normal(": Marvell SoC Real Time Clock\n");
	aprint_naive("\n");

	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh, mva->mva_offset,
	   mva->mva_size, &sc->sc_ioh)) {
		aprint_error_dev(self, "failed to subregion register space\n");
		return;
	}

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = NULL;
	sc->sc_todr.todr_settime = NULL;
	sc->sc_todr.todr_gettime_ymdhms = mvsocrtc_todr_gettime;
	sc->sc_todr.todr_settime_ymdhms = mvsocrtc_todr_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

static int
mvsocrtc_todr_gettime(todr_chip_handle_t ch, struct clock_ymdhms *dt)
{
	struct mvsocrtc_softc * const sc = ch->cookie;
	bool tried = false;
	uint32_t rtcdate, rtctime;
	uint8_t rtcday, rtchour, rtcmin, rtcmonth, rtcsec, rtcyear;

again:
	/* read the rtc date and time registers */
	rtcdate = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSOCRTC_DATE);
	rtctime = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSOCRTC_TIME);

	rtcyear = (rtcdate >> MVSOCRTC_YEAR_OFFSET) & MVSOCRTC_YEAR_MASK;
	rtcmonth = (rtcdate >> MVSOCRTC_MONTH_OFFSET) & MVSOCRTC_MONTH_MASK;
	rtcday = (rtcdate >> MVSOCRTC_DAY_OFFSET) & MVSOCRTC_DAY_MASK;

	rtchour = (rtctime >> MVSOCRTC_HOUR_OFFSET) & MVSOCRTC_HOUR_MASK;
	rtcmin = (rtctime >> MVSOCRTC_MINUTE_OFFSET) & MVSOCRTC_MINUTE_MASK;
	rtcsec = (rtctime >> MVSOCRTC_SECOND_OFFSET) & MVSOCRTC_SECOND_MASK;

	/*
	 * if seconds == 0, we may have read while the registers were
	 * updating.  Read again to get consistant data.
	 */
	if (rtcsec == 0 && !tried) {
		tried = true;
		goto again;
	}

	/*
	 * Assume year "00" to be year 2000.
	 * XXXX this assumption will fail in 2100, but somehow I don't think
	 * I or the hardware will be functioning to see it.
	 */
	dt->dt_year = bcdtobin(rtcyear) + 2000;
	dt->dt_mon = bcdtobin(rtcmonth);
	dt->dt_day = bcdtobin(rtcday);
	dt->dt_hour = bcdtobin(rtchour);
	dt->dt_min = bcdtobin(rtcmin);
	dt->dt_sec = bcdtobin(rtcsec);

	return 0;
}

static int
mvsocrtc_todr_settime(todr_chip_handle_t ch, struct clock_ymdhms *dt)
{
	struct mvsocrtc_softc * const sc = ch->cookie;
	uint32_t reg;

	/* compose & write time register contents */
	reg = (bintobcd(dt->dt_sec) << MVSOCRTC_SECOND_OFFSET) |
	   (bintobcd(dt->dt_min) << MVSOCRTC_MINUTE_OFFSET) |
	   (bintobcd(dt->dt_hour) << MVSOCRTC_HOUR_OFFSET) |
	   (bintobcd(dt->dt_wday) << MVSOCRTC_WDAY_OFFSET);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSOCRTC_TIME, reg);

	/* compose & write date register contents */
	reg = (bintobcd(dt->dt_day) << MVSOCRTC_DAY_OFFSET) |
	   (bintobcd(dt->dt_mon) << MVSOCRTC_MONTH_OFFSET) |
	   (bintobcd(dt->dt_year % 100) << MVSOCRTC_YEAR_OFFSET);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSOCRTC_DATE, reg);

	return 0;
}
