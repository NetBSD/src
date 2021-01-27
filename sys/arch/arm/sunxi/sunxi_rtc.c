/* $NetBSD: sunxi_rtc.c,v 1.10 2021/01/27 03:10:20 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_rtc.c,v 1.10 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/clock_subr.h>
#include <dev/clk/clk_backend.h>

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

#define	SUN6I_LOSC_CTRL_REG	0x00
#define	 SUN6I_LOSC_CTRL_KEY		(0x16aa << 16)
#define	 SUN6I_LOSC_CTRL_AUTO_SWT_BYPASS __BIT(15)
#define	 SUN6I_LOSC_CTRL_ALM_DHMS_ACC	__BIT(9)
#define	 SUN6I_LOSC_CTRL_RTC_HMS_ACC	__BIT(8)
#define	 SUN6I_LOSC_CTRL_RTC_YMD_ACC	__BIT(7)
#define	 SUN6I_LOSC_CTRL_EXT_LOSC_EN	__BIT(4)
#define	 SUN6I_LOSC_CTRL_EXT_OSC	__BIT(0)

#define	SUN6I_INTOSC_CLK_PRESCAL_REG 0x08
#define	 SUN6I_INTOSC_CLK_PRESCAL	__BITS(0,4)

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

#define	SUN6I_RTC_LOSC_OUT_GATING_REG 0x60
#define	 SUN6I_RTC_LOSC_OUT_EN	__BIT(0)

struct sunxi_rtc_config {
	bus_size_t	yy_mm_dd_reg;
	uint32_t	leap, year, month, day;
	bus_size_t	hh_mm_ss_reg;
	uint32_t	wk_no, hour, minute, second;
	u_int		base_year;

	u_int		iosc_rate;
	u_int		fixed_prescaler;
	uint32_t	ext_losc_en;
	uint32_t	auto_swt_bypass;
	u_int		flags;
};

#define	SUNXI_RTC_F_HAS_VAR_PRESCALER	__BIT(0)

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

static const struct sunxi_rtc_config sun6i_a31_rtc_config = {
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

	.iosc_rate = 667000,
	.flags = SUNXI_RTC_F_HAS_VAR_PRESCALER,
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

static const struct sunxi_rtc_config sun8i_a23_rtc_config = {
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

	.iosc_rate = 667000,
	.flags = SUNXI_RTC_F_HAS_VAR_PRESCALER,
};

static const struct sunxi_rtc_config sun8i_r40_rtc_config = {
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

	.iosc_rate = 16000000,
	.fixed_prescaler = 512,
};

static const struct sunxi_rtc_config sun8i_v3_rtc_config = {
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

	.iosc_rate = 32000,
};

static const struct sunxi_rtc_config sun8i_h3_rtc_config = {
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

	.iosc_rate = 16000000,
	.fixed_prescaler = 32,
	.flags = SUNXI_RTC_F_HAS_VAR_PRESCALER,
};

static const struct sunxi_rtc_config sun50i_h6_rtc_config = {
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

	.iosc_rate = 16000000,
	.fixed_prescaler = 32,
	.auto_swt_bypass = SUN6I_LOSC_CTRL_AUTO_SWT_BYPASS,
	.ext_losc_en = SUN6I_LOSC_CTRL_EXT_LOSC_EN,
	.flags = SUNXI_RTC_F_HAS_VAR_PRESCALER,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun4i-a10-rtc",
	  .data = &sun4i_rtc_config },
	{ .compat = "allwinner,sun6i-a31-rtc",
	  .data = &sun6i_a31_rtc_config },
	{ .compat = "allwinner,sun7i-a20-rtc",
	  .data = &sun7i_rtc_config },
	{ .compat = "allwinner,sun8i-a23-rtc",
	  .data = &sun8i_a23_rtc_config },
	{ .compat = "allwinner,sun8i-r40-rtc",
	  .data = &sun8i_r40_rtc_config },
	{ .compat = "allwinner,sun8i-v3-rtc",
	  .data = &sun8i_v3_rtc_config },
	{ .compat = "allwinner,sun8i-h3-rtc",
	  .data = &sun8i_h3_rtc_config },
	{ .compat = "allwinner,sun50i-h5-rtc",
	  .data = &sun8i_h3_rtc_config },
	{ .compat = "allwinner,sun50i-h6-rtc",
	  .data = &sun50i_h6_rtc_config },

	DEVICE_COMPAT_EOL
};

#define	SUNXI_RTC_CLK_LOSC	0
#define	SUNXI_RTC_CLK_LOSC_GATE	1
#define	SUNXI_RTC_CLK_IOSC	2
#define	SUNXI_RTC_NCLKS		3

struct sunxi_rtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct todr_chip_handle sc_todr;
	const struct sunxi_rtc_config *sc_conf;

	int sc_phandle;

	struct clk *sc_parent_clk;	/* external oscillator */

	/*
	 * We export up to 3 clocks:
	 * [0] The local oscillator output
	 * [1] Gated version of [0]
	 * [2] The internal oscillator
	 *
	 * The local oscillator is driven either by the internal
	 * oscillator (less precise) or an external oscillator.
	 *
	 * Note that these are the order they appear in the device
	 * tree "clock-output-names" property for our node.  Not
	 * all flavors of the Allwinner SoCs export all of these
	 * clocks, so we export only those that appear in the
	 * "clock-output-names" property.
	 */
	const char *sc_clk_names[SUNXI_RTC_NCLKS];
	struct clk sc_clks[SUNXI_RTC_NCLKS];
	kmutex_t sc_clk_mutex;
	struct clk_domain sc_clkdom;
};

#define RTC_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RTC_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	sunxi_rtc_match(device_t, cfdata_t, void *);
static void	sunxi_rtc_attach(device_t, device_t, void *);

static int	sunxi_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	sunxi_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

static struct clk *
		sunxi_rtc_clk_get(void *, const char *);
static u_int	sunxi_rtc_clk_get_rate(void *, struct clk *);
static int	sunxi_rtc_clk_enable(void *, struct clk *);
static int	sunxi_rtc_clk_disable(void *, struct clk *);
static int	sunxi_rtc_clk_set_parent(void *, struct clk *, struct clk *);
static struct clk *
		sunxi_rtc_clk_get_parent(void *, struct clk *);

static const struct clk_funcs sunxi_rtc_clk_funcs = {
	.get = sunxi_rtc_clk_get,
	.get_rate = sunxi_rtc_clk_get_rate,
	.enable = sunxi_rtc_clk_enable,
	.disable = sunxi_rtc_clk_disable,
	.set_parent = sunxi_rtc_clk_set_parent,
	.get_parent = sunxi_rtc_clk_get_parent,
};

static struct clk *
sunxi_rtc_clock_decode(device_t dev, int cc_phandle, const void *data,
		       size_t len)
{
	struct sunxi_rtc_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;
	
	const u_int clock_id = be32dec(data);
	if (clock_id >= SUNXI_RTC_NCLKS)
		return NULL;
	
	if (sc->sc_clk_names[clock_id] == NULL)
		return NULL;
	
	return &sc->sc_clks[clock_id];
}

static const struct fdtbus_clock_controller_func sunxi_rtc_fdtclock_funcs = {
	.decode = sunxi_rtc_clock_decode,
};

CFATTACH_DECL_NEW(sunxi_rtc, sizeof(struct sunxi_rtc_softc),
	sunxi_rtc_match, sunxi_rtc_attach, NULL, NULL);

static int
sunxi_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_rtc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_phandle = phandle;

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
	sc->sc_conf = of_compatible_lookup(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	mutex_init(&sc->sc_clk_mutex, MUTEX_DEFAULT, IPL_HIGH);

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = sunxi_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = sunxi_rtc_settime;

	fdtbus_todr_attach(self, phandle, &sc->sc_todr);

	sc->sc_parent_clk = fdtbus_clock_get_index(phandle, 0);

	if (sc->sc_parent_clk == NULL || sc->sc_conf->iosc_rate == 0)
		return;

	uint32_t reg = SUN6I_LOSC_CTRL_KEY;
	if (sc->sc_conf->auto_swt_bypass) {
		/*
		 * Disable auto-switching to the internal oscillator
		 * if the external oscillator disappears.
		 */
		reg |= sc->sc_conf->auto_swt_bypass;
		RTC_WRITE(sc, SUN6I_LOSC_CTRL_REG, reg);
	}

	/* Switch to the external oscillator by default. */
	reg |= SUN6I_LOSC_CTRL_EXT_OSC | sc->sc_conf->ext_losc_en;
	RTC_WRITE(sc, SUN6I_LOSC_CTRL_REG, reg);

	sc->sc_clkdom.name = device_xname(sc->sc_dev);
	sc->sc_clkdom.funcs = &sunxi_rtc_clk_funcs;
	sc->sc_clkdom.priv = sc;

	unsigned int i;
	for (i = 0; i < SUNXI_RTC_NCLKS; i++) {
		sc->sc_clk_names[i] = fdtbus_get_string_index(phandle,
		    "clock-output-names", i);
		if (sc->sc_clk_names[i] == NULL)
			break;
		sc->sc_clks[i].domain = &sc->sc_clkdom;
		sc->sc_clks[i].name = sc->sc_clk_names[i];
		clk_attach(&sc->sc_clks[i]);
	}

	fdtbus_register_clock_controller(sc->sc_dev, sc->sc_phandle,
	    &sunxi_rtc_fdtclock_funcs);
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
		aprint_normal_dev(sc->sc_dev, "year pre the epoch: %" PRIu64
		    ", not writing back time\n", dt->dt_year);
		return EIO;
	}
	maxyear = __SHIFTOUT(0xffffffff, conf->year) + conf->base_year;
	if (dt->dt_year > maxyear) {
		aprint_normal_dev(sc->sc_dev, "year exceeds available field:"
		    " %" PRIu64 ", not writing back time\n", dt->dt_year);
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

static struct clk *
sunxi_rtc_clk_get(void *priv, const char *name)
{
	struct sunxi_rtc_softc * const sc = priv;
	u_int i;

	for (i = 0; i < SUNXI_RTC_NCLKS; i++) {
		if (sc->sc_clk_names[i] != NULL &&
		    strcmp(sc->sc_clk_names[i], name) == 0) {
			return &sc->sc_clks[i];
		}
	}

	return NULL;
}

static u_int
sunxi_rtc_clk_get_rate(void *priv, struct clk *clk)
{
	struct sunxi_rtc_softc * const sc = priv;

	if (clk == &sc->sc_clks[SUNXI_RTC_CLK_IOSC]) {
		KASSERT(sc->sc_clk_names[SUNXI_RTC_CLK_IOSC] != NULL);
		KASSERT(sc->sc_conf->iosc_rate != 0);
		return sc->sc_conf->iosc_rate;
	}

	KASSERT(sc->sc_parent_clk != NULL);
	u_int parent_rate = clk_get_rate(sc->sc_parent_clk);
	uint32_t prescaler = 0;

	if (RTC_READ(sc, SUN6I_LOSC_CTRL_REG) & SUN6I_LOSC_CTRL_EXT_OSC)
		return parent_rate;

	if (sc->sc_conf->fixed_prescaler)
		parent_rate /= sc->sc_conf->fixed_prescaler;

	if (sc->sc_conf->flags & SUNXI_RTC_F_HAS_VAR_PRESCALER) {
		prescaler =
		    __SHIFTOUT(RTC_READ(sc, SUN6I_INTOSC_CLK_PRESCAL_REG),
			       SUN6I_INTOSC_CLK_PRESCAL);
	}

	return parent_rate / (prescaler + 1);
}

static int
sunxi_rtc_clk_enable(void *priv, struct clk *clk)
{
	struct sunxi_rtc_softc * const sc = priv;

	if (clk != &sc->sc_clks[SUNXI_RTC_CLK_LOSC_GATE])
		return 0;

	mutex_enter(&sc->sc_clk_mutex);
	uint32_t reg = RTC_READ(sc, SUN6I_RTC_LOSC_OUT_GATING_REG);
	if ((reg & SUN6I_RTC_LOSC_OUT_EN) == 0) {
		reg |= SUN6I_RTC_LOSC_OUT_EN;
		RTC_WRITE(sc, SUN6I_RTC_LOSC_OUT_GATING_REG, reg);
	}
	mutex_exit(&sc->sc_clk_mutex);

	return 0;
}

static int
sunxi_rtc_clk_disable(void *priv, struct clk *clk)
{
	struct sunxi_rtc_softc * const sc = priv;

	if (clk != &sc->sc_clks[SUNXI_RTC_CLK_LOSC_GATE])
		return EINVAL;

	mutex_enter(&sc->sc_clk_mutex);
	uint32_t reg = RTC_READ(sc, SUN6I_RTC_LOSC_OUT_GATING_REG);
	if (reg & SUN6I_RTC_LOSC_OUT_EN) {
		reg &= ~SUN6I_RTC_LOSC_OUT_EN;
		RTC_WRITE(sc, SUN6I_RTC_LOSC_OUT_GATING_REG, reg);
	}
	mutex_exit(&sc->sc_clk_mutex);

	return 0;
}

static int
sunxi_rtc_clk_set_parent(void *priv, struct clk *clk, struct clk *parent_clk)
{
	struct sunxi_rtc_softc * const sc = priv;

	if (clk == &sc->sc_clks[SUNXI_RTC_CLK_IOSC])
		return EINVAL;

	if (parent_clk != sc->sc_parent_clk &&
	    parent_clk != &sc->sc_clks[SUNXI_RTC_CLK_IOSC])
		return EINVAL;

	mutex_enter(&sc->sc_clk_mutex);
	uint32_t reg = RTC_READ(sc, SUN6I_LOSC_CTRL_REG);
	if (parent_clk == sc->sc_parent_clk)
		reg |= SUN6I_LOSC_CTRL_EXT_OSC | sc->sc_conf->ext_losc_en;
	else
		reg &= ~(SUN6I_LOSC_CTRL_EXT_OSC | sc->sc_conf->ext_losc_en);
	RTC_WRITE(sc, SUN6I_LOSC_CTRL_REG, reg | SUN6I_LOSC_CTRL_KEY);
	mutex_exit(&sc->sc_clk_mutex);

	return 0;
}

static struct clk *
sunxi_rtc_clk_get_parent(void *priv, struct clk *clk)
{
	struct sunxi_rtc_softc * const sc = priv;
	uint32_t reg;

	if (clk == &sc->sc_clks[SUNXI_RTC_CLK_IOSC])
		return NULL;

	reg = RTC_READ(sc, SUN6I_LOSC_CTRL_REG);
	if (reg & SUN6I_LOSC_CTRL_EXT_OSC)
		return sc->sc_parent_clk;

	/*
	 * We switch to the external oscillator at attach time becacuse
	 * it's higher quality than the internal one.  If we haven't
	 * exported the internal oscillator to the clock tree, then
	 * we shouldn't get here.
	 */
	KASSERT(sc->sc_clk_names[SUNXI_RTC_CLK_IOSC] != NULL);
	return &sc->sc_clks[SUNXI_RTC_CLK_IOSC];
}
