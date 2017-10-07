/* $NetBSD: sunxi_rtc.c,v 1.2 2017/10/07 17:03:49 jmcneill Exp $ */

/*-
 * Copyright (c) 2014-2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_rtc.c,v 1.2 2017/10/07 17:03:49 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/clock_subr.h>

#include <dev/fdt/fdtvar.h>

#define	SUN7I_RTC_YY_MM_DD_REG	0x04
#define	 SUN7I_RTC_LEAP		__BIT(24)
#define	 SUN7I_RTC_YEAR		__BITS(23,16)
#define	 SUN7I_RTC_MONTH	__BITS(11,8)
#define	 SUN7I_RTC_DAY		__BITS(4,0)
#define	SUN7I_RTC_HH_MM_SS_REG	0x08
#define	 SUN7I_RTC_WK_NO	__BITS(31,29)
#define	 SUN7I_RTC_HOUR		__BITS(20,16)
#define	 SUN7I_RTC_MINUTE	__BITS(13,8)
#define	 SUN7I_RTC_SECOND	__BITS(5,0)

#define	SUN6I_RTC_YY_MM_DD_REG	0x10
#define	 SUN6I_RTC_LEAP		__BIT(22)
#define	 SUN6I_RTC_YEAR		__BITS(21,16)
#define	 SUN6I_RTC_MONTH	__BITS(11,8)
#define	 SUN6I_RTC_DAY		__BITS(4,0)
#define	SUN6I_RTC_HH_MM_SS_REG	0x14
#define	 SUN6I_RTC_WK_NO	__BITS(31,29)
#define	 SUN6I_RTC_HOUR		__BITS(20,16)
#define	 SUN6I_RTC_MINUTE	__BITS(13,8)
#define	 SUN6I_RTC_SECOND	__BITS(5,0)

#define	SUNXI_RTC_BASE_YEAR	2000

enum sunxi_rtc_type {
	RTC_SUN6I = 1,
	RTC_SUN7I,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun6i-a31-rtc",		RTC_SUN6I },
	{ "allwinner,sun7i-a20-rtc",		RTC_SUN7I },
	{ NULL }
};

struct sunxi_rtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct todr_chip_handle sc_todr;
};

#define RTC_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RTC_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	sunxi_rtc_match(device_t, cfdata_t, void *);
static void	sunxi_rtc_attach(device_t, device_t, void *);

static int	sun6i_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	sun6i_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

static int	sun7i_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	sun7i_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

CFATTACH_DECL_NEW(sunxi_rtc, sizeof(struct sunxi_rtc_softc),
	sunxi_rtc_match, sunxi_rtc_attach, NULL, NULL);

static int
sunxi_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_rtc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	enum sunxi_rtc_type type;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	type = of_search_compatible(phandle, compat_data)->data;

	sc->sc_todr.cookie = sc;
	switch (type) {
	case RTC_SUN6I:
		sc->sc_todr.todr_gettime_ymdhms = sun6i_rtc_gettime;
		sc->sc_todr.todr_settime_ymdhms = sun6i_rtc_settime;
		break;
	case RTC_SUN7I:
		sc->sc_todr.todr_gettime_ymdhms = sun7i_rtc_gettime;
		sc->sc_todr.todr_settime_ymdhms = sun7i_rtc_settime;
		break;
	}

	fdtbus_todr_attach(self, phandle, &sc->sc_todr);
}

static int
sun6i_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;

	const uint32_t yymmdd = RTC_READ(sc, SUN6I_RTC_YY_MM_DD_REG);
	const uint32_t hhmmss = RTC_READ(sc, SUN6I_RTC_HH_MM_SS_REG);

	dt->dt_year = __SHIFTOUT(yymmdd, SUN6I_RTC_YEAR) + SUNXI_RTC_BASE_YEAR;
	dt->dt_mon = __SHIFTOUT(yymmdd, SUN6I_RTC_MONTH);
	dt->dt_day = __SHIFTOUT(yymmdd, SUN6I_RTC_DAY);
	dt->dt_wday = __SHIFTOUT(hhmmss, SUN6I_RTC_WK_NO);
	dt->dt_hour = __SHIFTOUT(hhmmss, SUN6I_RTC_HOUR);
	dt->dt_min = __SHIFTOUT(hhmmss, SUN6I_RTC_MINUTE);
	dt->dt_sec = __SHIFTOUT(hhmmss, SUN6I_RTC_SECOND);

	return 0;
}

static int
sun6i_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;
	uint32_t yymmdd, hhmmss, maxyear;

	/*
	 * Sanity check the date before writing it back
	 */
	if (dt->dt_year < SUNXI_RTC_BASE_YEAR) {
		aprint_normal_dev(sc->sc_dev, "year pre the epoch: %llu, "
		    "not writing back time\n", dt->dt_year);
		return EIO;
	}
	maxyear = __SHIFTOUT(0xffffffff, SUN6I_RTC_YEAR) + SUNXI_RTC_BASE_YEAR;
	if (dt->dt_year > maxyear) {
		aprint_normal_dev(sc->sc_dev, "year exceeds available field:"
		    " %llu, not writing back time\n", dt->dt_year);
		return EIO;
	}

	yymmdd = __SHIFTIN(dt->dt_year - SUNXI_RTC_BASE_YEAR, SUN6I_RTC_YEAR);
	yymmdd |= __SHIFTIN(dt->dt_mon, SUN6I_RTC_MONTH);
	yymmdd |= __SHIFTIN(dt->dt_day, SUN6I_RTC_DAY);

	hhmmss = __SHIFTIN(dt->dt_wday, SUN6I_RTC_WK_NO);
	hhmmss |= __SHIFTIN(dt->dt_hour, SUN6I_RTC_HOUR);
	hhmmss |= __SHIFTIN(dt->dt_min, SUN6I_RTC_MINUTE);
	hhmmss |= __SHIFTIN(dt->dt_sec, SUN6I_RTC_SECOND);

	RTC_WRITE(sc, SUN6I_RTC_YY_MM_DD_REG, yymmdd);
	RTC_WRITE(sc, SUN6I_RTC_HH_MM_SS_REG, hhmmss);

	return 0;
}

static int
sun7i_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;

	const uint32_t yymmdd = RTC_READ(sc, SUN7I_RTC_YY_MM_DD_REG);
	const uint32_t hhmmss = RTC_READ(sc, SUN7I_RTC_HH_MM_SS_REG);

	dt->dt_year = __SHIFTOUT(yymmdd, SUN7I_RTC_YEAR) + SUNXI_RTC_BASE_YEAR;
	dt->dt_mon = __SHIFTOUT(yymmdd, SUN7I_RTC_MONTH);
	dt->dt_day = __SHIFTOUT(yymmdd, SUN7I_RTC_DAY);
	dt->dt_wday = __SHIFTOUT(hhmmss, SUN7I_RTC_WK_NO);
	dt->dt_hour = __SHIFTOUT(hhmmss, SUN7I_RTC_HOUR);
	dt->dt_min = __SHIFTOUT(hhmmss, SUN7I_RTC_MINUTE);
	dt->dt_sec = __SHIFTOUT(hhmmss, SUN7I_RTC_SECOND);

	return 0;
}

static int
sun7i_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;
	uint32_t yymmdd, hhmmss, maxyear;

	/*
	 * Sanity check the date before writing it back
	 */
	if (dt->dt_year < SUNXI_RTC_BASE_YEAR) {
		aprint_normal_dev(sc->sc_dev, "year pre the epoch: %llu, "
		    "not writing back time\n", dt->dt_year);
		return EIO;
	}
	maxyear = __SHIFTOUT(0xffffffff, SUN7I_RTC_YEAR) + SUNXI_RTC_BASE_YEAR;
	if (dt->dt_year > maxyear) {
		aprint_normal_dev(sc->sc_dev, "year exceeds available field:"
		    " %llu, not writing back time\n", dt->dt_year);
		return EIO;
	}

	yymmdd = __SHIFTIN(dt->dt_year - SUNXI_RTC_BASE_YEAR, SUN7I_RTC_YEAR);
	yymmdd |= __SHIFTIN(dt->dt_mon, SUN7I_RTC_MONTH);
	yymmdd |= __SHIFTIN(dt->dt_day, SUN7I_RTC_DAY);

	hhmmss = __SHIFTIN(dt->dt_wday, SUN7I_RTC_WK_NO);
	hhmmss |= __SHIFTIN(dt->dt_hour, SUN7I_RTC_HOUR);
	hhmmss |= __SHIFTIN(dt->dt_min, SUN7I_RTC_MINUTE);
	hhmmss |= __SHIFTIN(dt->dt_sec, SUN7I_RTC_SECOND);

	RTC_WRITE(sc, SUN7I_RTC_YY_MM_DD_REG, yymmdd);
	RTC_WRITE(sc, SUN7I_RTC_HH_MM_SS_REG, hhmmss);

	return 0;
}
