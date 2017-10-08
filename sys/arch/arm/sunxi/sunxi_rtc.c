/* $NetBSD: sunxi_rtc.c,v 1.3 2017/10/08 14:03:46 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_rtc.c,v 1.3 2017/10/08 14:03:46 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/clock_subr.h>

#include <dev/fdt/fdtvar.h>

#define	SUN4I_RTC_YY_MM_DD_REG	0x04
#define	 SUN4I_RTC_LEAP		__BIT(22)
#define	 SUN4I_RTC_YEAR		__BITS(21,16)
#define	 SUN4I_RTC_MONTH	__BITS(11,8)
#define	 SUN4I_RTC_DAY		__BITS(4,0)
#define	SUN4I_RTC_HH_MM_SS_REG	0x08
#define	 SUN4I_RTC_WK_NO	__BITS(31,29)
#define	 SUN4I_RTC_HOUR		__BITS(20,16)
#define	 SUN4I_RTC_MINUTE	__BITS(13,8)
#define	 SUN4I_RTC_SECOND	__BITS(5,0)
#define	SUN4I_RTC_BASE_YEAR	2010

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
#define	SUN7I_RTC_BASE_YEAR	1970

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
#define	SUN6I_RTC_BASE_YEAR	2000

struct sunxi_rtc_config {
	bus_size_t	yy_mm_dd_reg;
	uint32_t	leap, year, month, day;
	bus_size_t	hh_mm_ss_reg;
	uint32_t	wk_no, hour, minute, second;
	u_int		base_year;
};

static const struct sunxi_rtc_config sun4i_rtc_config = {
	.yy_mm_dd_reg = SUN4I_RTC_YY_MM_DD_REG,
	.leap = SUN4I_RTC_LEAP,
	.year = SUN4I_RTC_YEAR,
	.month = SUN4I_RTC_MONTH,
	.day = SUN4I_RTC_DAY,
	.hh_mm_ss_reg = SUN4I_RTC_HH_MM_SS_REG,
	.wk_no = SUN4I_RTC_WK_NO,
	.hour = SUN4I_RTC_HOUR,
	.minute = SUN4I_RTC_MINUTE,
	.second = SUN4I_RTC_SECOND,
	.base_year = SUN4I_RTC_BASE_YEAR,
};

static const struct sunxi_rtc_config sun6i_rtc_config = {
	.yy_mm_dd_reg = SUN6I_RTC_YY_MM_DD_REG,
	.leap = SUN6I_RTC_LEAP,
	.year = SUN6I_RTC_YEAR,
	.month = SUN6I_RTC_MONTH,
	.day = SUN6I_RTC_DAY,
	.hh_mm_ss_reg = SUN6I_RTC_HH_MM_SS_REG,
	.wk_no = SUN6I_RTC_WK_NO,
	.hour = SUN6I_RTC_HOUR,
	.minute = SUN6I_RTC_MINUTE,
	.second = SUN6I_RTC_SECOND,
	.base_year = SUN6I_RTC_BASE_YEAR,
};

static const struct sunxi_rtc_config sun7i_rtc_config = {
	.yy_mm_dd_reg = SUN7I_RTC_YY_MM_DD_REG,
	.leap = SUN7I_RTC_LEAP,
	.year = SUN7I_RTC_YEAR,
	.month = SUN7I_RTC_MONTH,
	.day = SUN7I_RTC_DAY,
	.hh_mm_ss_reg = SUN7I_RTC_HH_MM_SS_REG,
	.wk_no = SUN7I_RTC_WK_NO,
	.hour = SUN7I_RTC_HOUR,
	.minute = SUN7I_RTC_MINUTE,
	.second = SUN7I_RTC_SECOND,
	.base_year = SUN7I_RTC_BASE_YEAR,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-rtc",		(uintptr_t)&sun4i_rtc_config },
	{ "allwinner,sun6i-a31-rtc",		(uintptr_t)&sun6i_rtc_config },
	{ "allwinner,sun7i-a20-rtc",		(uintptr_t)&sun7i_rtc_config },
	{ NULL }
};

struct sunxi_rtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct todr_chip_handle sc_todr;
	const struct sunxi_rtc_config *sc_conf;
};

#define RTC_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RTC_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	sunxi_rtc_match(device_t, cfdata_t, void *);
static void	sunxi_rtc_attach(device_t, device_t, void *);

static int	sunxi_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	sunxi_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

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
	sc->sc_conf = (void *)of_search_compatible(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = sunxi_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = sunxi_rtc_settime;

	fdtbus_todr_attach(self, phandle, &sc->sc_todr);
}

static int
sunxi_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;
	const struct sunxi_rtc_config *conf = sc->sc_conf;

	const uint32_t yymmdd = RTC_READ(sc, conf->yy_mm_dd_reg);
	const uint32_t hhmmss = RTC_READ(sc, conf->hh_mm_ss_reg);

	dt->dt_year = __SHIFTOUT(yymmdd, conf->year) + conf->base_year;
	dt->dt_mon = __SHIFTOUT(yymmdd, conf->month);
	dt->dt_day = __SHIFTOUT(yymmdd, conf->day);
	dt->dt_wday = __SHIFTOUT(hhmmss, conf->wk_no);
	dt->dt_hour = __SHIFTOUT(hhmmss, conf->hour);
	dt->dt_min = __SHIFTOUT(hhmmss, conf->minute);
	dt->dt_sec = __SHIFTOUT(hhmmss, conf->second);

	return 0;
}

static int
sunxi_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;
	const struct sunxi_rtc_config *conf = sc->sc_conf;
	uint32_t yymmdd, hhmmss, maxyear;

	/*
	 * Sanity check the date before writing it back
	 */
	if (dt->dt_year < conf->base_year) {
		aprint_normal_dev(sc->sc_dev, "year pre the epoch: %llu, "
		    "not writing back time\n", dt->dt_year);
		return EIO;
	}
	maxyear = __SHIFTOUT(0xffffffff, conf->year) + conf->base_year;
	if (dt->dt_year > maxyear) {
		aprint_normal_dev(sc->sc_dev, "year exceeds available field:"
		    " %llu, not writing back time\n", dt->dt_year);
		return EIO;
	}

	yymmdd = __SHIFTIN(dt->dt_year - conf->base_year, conf->year);
	yymmdd |= __SHIFTIN(dt->dt_mon, conf->month);
	yymmdd |= __SHIFTIN(dt->dt_day, conf->day);

	hhmmss = __SHIFTIN(dt->dt_wday, conf->wk_no);
	hhmmss |= __SHIFTIN(dt->dt_hour, conf->hour);
	hhmmss |= __SHIFTIN(dt->dt_min, conf->minute);
	hhmmss |= __SHIFTIN(dt->dt_sec, conf->second);

	RTC_WRITE(sc, conf->yy_mm_dd_reg, yymmdd);
	RTC_WRITE(sc, conf->hh_mm_ss_reg, hhmmss);

	return 0;
}
