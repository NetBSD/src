/* $NetBSD: sunxi_rtc.c,v 1.1.6.2 2017/08/28 17:51:32 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_rtc.c,v 1.1.6.2 2017/08/28 17:51:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/clock_subr.h>

#include <dev/fdt/fdtvar.h>

#define	RTC_YY_MM_DD_REG	0x10
#define	 SUNXI_RTC_LEAP		__BIT(22)
#define	 SUNXI_RTC_YEAR		__BITS(21,16)
#define	 SUNXI_RTC_MONTH	__BITS(11,8)
#define	 SUNXI_RTC_DAY		__BITS(4,0)
#define	RTC_HH_MM_SS_REG	0x14
#define	 SUNXI_RTC_WK_NO	__BITS(31,29)
#define	 SUNXI_RTC_HOUR		__BITS(20,16)
#define	 SUNXI_RTC_MINUTE	__BITS(13,8)
#define	 SUNXI_RTC_SECOND	__BITS(5,0)

#define	SUNXI_RTC_BASE_YEAR	2000

static const char * const compatible[] = {
	"allwinner,sun6i-a31-rtc",
	NULL
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

static int	sunxi_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	sunxi_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

CFATTACH_DECL_NEW(sunxi_rtc, sizeof(struct sunxi_rtc_softc),
	sunxi_rtc_match, sunxi_rtc_attach, NULL, NULL);

static int
sunxi_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
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

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	sc->sc_todr.todr_gettime_ymdhms = sunxi_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = sunxi_rtc_settime;
	sc->sc_todr.cookie = sc;

	fdtbus_todr_attach(self, phandle, &sc->sc_todr);
}

static int
sunxi_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct sunxi_rtc_softc *sc = tch->cookie;

	const uint32_t yymmdd = RTC_READ(sc, RTC_YY_MM_DD_REG);
	const uint32_t hhmmss = RTC_READ(sc, RTC_HH_MM_SS_REG);

	dt->dt_year = __SHIFTOUT(yymmdd, SUNXI_RTC_YEAR) + SUNXI_RTC_BASE_YEAR;
	dt->dt_mon = __SHIFTOUT(yymmdd, SUNXI_RTC_MONTH);
	dt->dt_day = __SHIFTOUT(yymmdd, SUNXI_RTC_DAY);
	dt->dt_wday = __SHIFTOUT(hhmmss, SUNXI_RTC_WK_NO);
	dt->dt_hour = __SHIFTOUT(hhmmss, SUNXI_RTC_HOUR);
	dt->dt_min = __SHIFTOUT(hhmmss, SUNXI_RTC_MINUTE);
	dt->dt_sec = __SHIFTOUT(hhmmss, SUNXI_RTC_SECOND);

	return 0;
}

static int
sunxi_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
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
	maxyear = __SHIFTOUT(0xffffffff, SUNXI_RTC_YEAR) + SUNXI_RTC_BASE_YEAR;
	if (dt->dt_year > maxyear) {
		aprint_normal_dev(sc->sc_dev, "year exceeds available field:"
		    " %llu, not writing back time\n", dt->dt_year);
		return EIO;
	}

	yymmdd = __SHIFTIN(dt->dt_year - SUNXI_RTC_BASE_YEAR, SUNXI_RTC_YEAR);
	yymmdd |= __SHIFTIN(dt->dt_mon, SUNXI_RTC_MONTH);
	yymmdd |= __SHIFTIN(dt->dt_day, SUNXI_RTC_DAY);

	hhmmss = __SHIFTIN(dt->dt_wday, SUNXI_RTC_WK_NO);
	hhmmss |= __SHIFTIN(dt->dt_hour, SUNXI_RTC_HOUR);
	hhmmss |= __SHIFTIN(dt->dt_min, SUNXI_RTC_MINUTE);
	hhmmss |= __SHIFTIN(dt->dt_sec, SUNXI_RTC_SECOND);

	RTC_WRITE(sc, RTC_YY_MM_DD_REG, yymmdd);
	RTC_WRITE(sc, RTC_HH_MM_SS_REG, hhmmss);

	return 0;
}
