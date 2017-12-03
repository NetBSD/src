/* $NetBSD: awin_rtc.c,v 1.7.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_rtc.c,v 1.7.20.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/clock_subr.h>

struct awin_rtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct todr_chip_handle sc_todr;
	uint32_t sc_loscctrl_reg;
	uint32_t sc_yymmdd_reg;
	uint32_t sc_hhmmss_reg;
	uint32_t sc_year_base;
	uint32_t sc_year_mask;
};

#define RTC_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RTC_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_rtc_match(device_t, cfdata_t, void *);
static void	awin_rtc_attach(device_t, device_t, void *);

static int	awin_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	awin_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

CFATTACH_DECL_NEW(awin_rtc, sizeof(struct awin_rtc_softc),
	awin_rtc_match, awin_rtc_attach, NULL, NULL);

static int
awin_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct awin_rtc_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		sc->sc_loscctrl_reg = AWIN_A31_LOSC_CTRL_REG;
		sc->sc_yymmdd_reg = AWIN_A31_RTC_YY_MM_DD_REG;
		sc->sc_hhmmss_reg = AWIN_A31_RTC_HH_MM_SS_REG;
	} else {
		sc->sc_loscctrl_reg = AWIN_LOSC_CTRL_REG;
		sc->sc_yymmdd_reg = AWIN_RTC_YY_MM_DD_REG;
		sc->sc_hhmmss_reg = AWIN_RTC_HH_MM_SS_REG;
	}

	if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		sc->sc_year_base = 1900;
		sc->sc_year_mask = AWIN_A20_RTC_YY_MM_DD_YEAR;
	} else {
		sc->sc_year_base = POSIX_BASE_YEAR;
		sc->sc_year_mask = AWIN_RTC_YY_MM_DD_YEAR;
	}
#ifdef AWIN_RTC_BASE_YEAR
	sc->sc_year_base = AWIN_RTC_BASE_YEAR;
#endif

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	sc->sc_todr.todr_gettime_ymdhms = awin_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = awin_rtc_settime;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);
}

static int
awin_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct awin_rtc_softc *sc = tch->cookie;
	uint32_t yymmdd, hhmmss;

	yymmdd = RTC_READ(sc, sc->sc_yymmdd_reg);
	hhmmss = RTC_READ(sc, sc->sc_hhmmss_reg);

	dt->dt_year = __SHIFTOUT(yymmdd, sc->sc_year_mask) + sc->sc_year_base;
	dt->dt_mon = __SHIFTOUT(yymmdd, AWIN_RTC_YY_MM_DD_MONTH);
	dt->dt_day = __SHIFTOUT(yymmdd, AWIN_RTC_YY_MM_DD_DAY);
	dt->dt_wday = __SHIFTOUT(hhmmss, AWIN_RTC_HH_MM_SS_WK_NO);
	dt->dt_hour = __SHIFTOUT(hhmmss, AWIN_RTC_HH_MM_SS_HOUR);
	dt->dt_min = __SHIFTOUT(hhmmss, AWIN_RTC_HH_MM_SS_MINUTE);
	dt->dt_sec = __SHIFTOUT(hhmmss, AWIN_RTC_HH_MM_SS_SECOND);

	return 0;
}

static int
awin_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct awin_rtc_softc *sc = tch->cookie;
	uint32_t yymmdd, hhmmss, losc, maxyear;

	losc = RTC_READ(sc, sc->sc_loscctrl_reg);
	if (losc & AWIN_LOSC_CTRL_BUSY)
		return EBUSY;

	/*
	 * Sanity check the date before writing it back
	 */
	if (dt->dt_year < POSIX_BASE_YEAR) {
		aprint_normal_dev(sc->sc_dev, "year pre the epoch: %llu, "
		    "not writing back time\n", dt->dt_year);
		return EIO;
	}
	maxyear = __SHIFTOUT(0xffffffff, sc->sc_year_mask) + sc->sc_year_base;
	if (dt->dt_year > maxyear) {
		aprint_normal_dev(sc->sc_dev, "year exceeds available field:"
		    " %llu, not writing back time\n", dt->dt_year);
		return EIO;
	}

	yymmdd = __SHIFTIN(dt->dt_year - sc->sc_year_base,
	    sc->sc_year_mask);

	KASSERT(__SHIFTOUT(yymmdd, sc->sc_year_mask) +
	    sc->sc_year_base == dt->dt_year);

	yymmdd |= __SHIFTIN(dt->dt_mon, AWIN_RTC_YY_MM_DD_MONTH);
	yymmdd |= __SHIFTIN(dt->dt_day, AWIN_RTC_YY_MM_DD_DAY);

	hhmmss = 0;
	hhmmss |= __SHIFTIN(dt->dt_wday, AWIN_RTC_HH_MM_SS_WK_NO);
	hhmmss |= __SHIFTIN(dt->dt_hour, AWIN_RTC_HH_MM_SS_HOUR);
	hhmmss |= __SHIFTIN(dt->dt_min, AWIN_RTC_HH_MM_SS_MINUTE);
	hhmmss |= __SHIFTIN(dt->dt_sec, AWIN_RTC_HH_MM_SS_SECOND);

	RTC_WRITE(sc, sc->sc_yymmdd_reg, yymmdd);
	RTC_WRITE(sc, sc->sc_hhmmss_reg, hhmmss);

	return 0;
}
