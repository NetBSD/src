/* $NetBSD: meson_pwm.c,v 1.5 2021/01/27 03:10:18 thorpej Exp $ */

/*
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_pwm.c,v 1.5 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/pwm/pwmvar.h>

/*#define MESON_PWM_DEBUG*/

#define MESON_PWM_DUTYCYCLE_A_REG	0x00
#define MESON_PWM_DUTYCYCLE_B_REG	0x01
#define  MESON_PWM_DUTYCYCLE_HIGH	__BITS(31,16)
#define  MESON_PWM_DUTYCYCLE_LOW	__BITS(15,0)
#define MESON_PWM_MISC_AB_REG		0x02
#define  MESON_PWM_MISC_AB_B_CLK_EN	__BIT(23)
#define  MESON_PWM_MISC_AB_B_CLK_DIV	__BITS(22,16)
#define  MESON_PWM_MISC_AB_A_CLK_EN	__BIT(15)
#define  MESON_PWM_MISC_AB_A_CLK_DIV	__BITS(14,8)
#define  MESON_PWM_MISC_AB_B_CLK_SEL	__BITS(7,6)
#define  MESON_PWM_MISC_AB_A_CLK_SEL	__BITS(5,4)
#define  MESON_PWM_MISC_AB_DS_B_EN	__BIT(3)
#define  MESON_PWM_MISC_AB_DS_A_EN	__BIT(2)
#define  MESON_PWM_MISC_AB_PWM_B_EN	__BIT(1)
#define  MESON_PWM_MISC_AB_PWM_A_EN	__BIT(0)
#define MESON_PWM_DELTASIGMA_A_B_REG	0x03
#define MESON_PWM_TIME_AB_REG		0x04
#define MESON_PWM_A2_REG		0x05
#define MESON_PWM_B2_REG		0x06
#define MESON_PWM_BLINK_AB_REG		0x07

#define PWM_READ_REG(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg) * 4)
#define PWM_WRITE_REG(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg) * 4, (val))

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-g12a-ao-pwm-ab" },
	{ .compat = "amlogic,meson-g12a-ao-pwm-cd" },
	{ .compat = "amlogic,meson-g12a-ee-pwm" },
	DEVICE_COMPAT_EOL
};

#define MESON_PWM_NCHAN	2
struct meson_pwm_channel {
	struct meson_pwm_softc *mpc_sc;
	struct pwm_controller mpc_pwm;
	struct pwm_config mpc_conf;
	u_int mpc_index;
};

struct meson_pwm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_reglock;	/* for PWM_A vs. PWM_B */
	int sc_phandle;
	u_int sc_clkfreq;
	u_int sc_clksource;
	struct meson_pwm_channel sc_pwmchan[MESON_PWM_NCHAN];
};

static int
meson_pwm_enable(pwm_tag_t pwm, bool enable)
{
	struct meson_pwm_channel * const pwmchan = pwm->pwm_priv;
	struct meson_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t val;

	mutex_enter(&sc->sc_reglock);
	switch (pwmchan->mpc_index) {
	case 0: /* A */
		val = PWM_READ_REG(sc, MESON_PWM_MISC_AB_REG);
		val &= ~MESON_PWM_MISC_AB_DS_A_EN;
		val |= MESON_PWM_MISC_AB_PWM_A_EN;
		PWM_WRITE_REG(sc, MESON_PWM_MISC_AB_REG, val);
		break;
	case 1: /* B */
		val = PWM_READ_REG(sc, MESON_PWM_MISC_AB_REG);
		val &= ~MESON_PWM_MISC_AB_DS_B_EN;
		val |= MESON_PWM_MISC_AB_PWM_B_EN;
		PWM_WRITE_REG(sc, MESON_PWM_MISC_AB_REG, val);
		break;
	}
	mutex_exit(&sc->sc_reglock);

	return 0;
}

static void
meson_pwm_get_current(struct meson_pwm_softc *sc, int chan,
    struct pwm_config *conf)
{
	uint64_t period_hz, duty_hz;
	uint32_t val;
	u_int period, duty, clk_div, hi, lo;

	memset(conf, 0, sizeof(*conf));

	mutex_enter(&sc->sc_reglock);
	switch (chan) {
	case 0: /* A */
		val = PWM_READ_REG(sc, MESON_PWM_MISC_AB_REG);
		clk_div = __SHIFTOUT(val, MESON_PWM_MISC_AB_A_CLK_DIV);
		val = PWM_READ_REG(sc, MESON_PWM_DUTYCYCLE_A_REG);
		hi = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_HIGH);
		lo = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_LOW);
		break;
	case 1: /* B */
		val = PWM_READ_REG(sc, MESON_PWM_MISC_AB_REG);
		clk_div = __SHIFTOUT(val, MESON_PWM_MISC_AB_B_CLK_DIV);
		val = PWM_READ_REG(sc, MESON_PWM_DUTYCYCLE_B_REG);
		hi = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_HIGH);
		lo = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_LOW);
		break;
	default:
		mutex_exit(&sc->sc_reglock);
		return;
	}
	mutex_exit(&sc->sc_reglock);

	clk_div += 1;
	duty_hz = (uint64_t)hi * clk_div;
	period_hz = (uint64_t)(hi + lo) * clk_div;

	period = period_hz * 1000000000ULL / sc->sc_clkfreq;
	duty = duty_hz * 1000000000ULL / sc->sc_clkfreq;

	conf->polarity = PWM_ACTIVE_HIGH;
	conf->period = period;
	conf->duty_cycle = duty;
}

static int
meson_pwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	struct meson_pwm_channel * const pwmchan = pwm->pwm_priv;

	*conf = pwmchan->mpc_conf;
	return 0;
}

static int
meson_pwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	struct meson_pwm_channel * const pwmchan = pwm->pwm_priv;
	struct meson_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint64_t period_hz, duty_hz;
	uint32_t val;
	u_int period, duty, clk_div, hi, lo;
#ifdef MESON_PWM_DEBUG
	u_int old_div = 0, old_hi = 0, old_lo = 0;
#endif

	period = conf->period;
	duty = conf->duty_cycle;
	if (period == 0)
		return EINVAL;
	KASSERT(period >= duty);
	if (conf->polarity == PWM_ACTIVE_LOW)
		duty = period - duty;

	/* calculate the period to be within the maximum value (0xffff) */
#define MESON_PWMTIME_MAX	0xffff
#define MESON_CLKDIV_MAX	127
	clk_div = 1;
	period_hz = ((uint64_t)sc->sc_clkfreq * period + 500000000ULL) /
	    1000000000ULL;
	duty_hz = ((uint64_t)sc->sc_clkfreq * duty + 500000000ULL) /
	    1000000000ULL;
	if (period_hz > MESON_PWMTIME_MAX) {
		clk_div = (period_hz + 0x7fff) / 0xffff;
		period_hz /= clk_div;
		duty_hz /= clk_div;
	}

	clk_div -= 1;	/* the divider is N+1 */
	if (clk_div > MESON_CLKDIV_MAX)
		return EINVAL;

	hi = duty_hz;
	lo = period_hz - duty_hz;

	mutex_enter(&sc->sc_reglock);
	switch (pwmchan->mpc_index) {
	case 0: /* A */
		val = PWM_READ_REG(sc, MESON_PWM_MISC_AB_REG);
		val &= ~MESON_PWM_MISC_AB_A_CLK_DIV;
#ifdef MESON_PWM_DEBUG
		old_div = __SHIFTOUT(val, MESON_PWM_MISC_AB_A_CLK_DIV);
#endif
		val |= __SHIFTIN(clk_div, MESON_PWM_MISC_AB_A_CLK_DIV);
		val &= ~MESON_PWM_MISC_AB_A_CLK_SEL;
		val |= __SHIFTIN(sc->sc_clksource, MESON_PWM_MISC_AB_A_CLK_SEL);
		val |= MESON_PWM_MISC_AB_A_CLK_EN;
		PWM_WRITE_REG(sc, MESON_PWM_MISC_AB_REG, val);
		val = PWM_READ_REG(sc, MESON_PWM_DUTYCYCLE_A_REG);
#ifdef MESON_PWM_DEBUG
		old_hi = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_HIGH);
		old_lo = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_LOW);
#endif
		PWM_WRITE_REG(sc, MESON_PWM_DUTYCYCLE_A_REG,
		    __SHIFTIN(hi, MESON_PWM_DUTYCYCLE_HIGH) |
		    __SHIFTIN(lo, MESON_PWM_DUTYCYCLE_LOW));
		break;
	case 1: /* B */
		val = PWM_READ_REG(sc, MESON_PWM_MISC_AB_REG);
		val &= ~MESON_PWM_MISC_AB_B_CLK_DIV;
#ifdef MESON_PWM_DEBUG
		old_div = __SHIFTOUT(val, MESON_PWM_MISC_AB_B_CLK_DIV);
#endif
		val |= __SHIFTIN(clk_div, MESON_PWM_MISC_AB_B_CLK_DIV);
		val &= ~MESON_PWM_MISC_AB_B_CLK_SEL;
		val |= __SHIFTIN(sc->sc_clksource, MESON_PWM_MISC_AB_B_CLK_SEL);
		val |= MESON_PWM_MISC_AB_B_CLK_EN;
		PWM_WRITE_REG(sc, MESON_PWM_MISC_AB_REG, val);
		val = PWM_READ_REG(sc, MESON_PWM_DUTYCYCLE_B_REG);
#ifdef MESON_PWM_DEBUG
		old_hi = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_HIGH);
		old_lo = __SHIFTOUT(val, MESON_PWM_DUTYCYCLE_LOW);
#endif
		PWM_WRITE_REG(sc, MESON_PWM_DUTYCYCLE_B_REG,
		    __SHIFTIN(hi, MESON_PWM_DUTYCYCLE_HIGH) |
		    __SHIFTIN(lo, MESON_PWM_DUTYCYCLE_LOW));
		break;
	}
	mutex_exit(&sc->sc_reglock);

	pwmchan->mpc_conf = *conf;

#ifdef MESON_PWM_DEBUG
	device_printf(sc->sc_dev,
	    "%s: %s: polarity=%s, DutuCycle/Period=%uns/%uns(%u%%) : "
	    "%uHz, HIGH:LOW=%u:%u(%u%%) -> "
	    "%uHz, HIGH:LOW=%u:%u(%u%%)\n", __func__,
	    pwmchan->mpc_index ? "A" : "B",
	    (conf->polarity == PWM_ACTIVE_LOW) ? "LOW" : "HIGH",
	    conf->duty_cycle, conf->period,
	    conf->duty_cycle * 100 / conf->period,
	    sc->sc_clkfreq / (old_div + 1), old_hi, old_lo,
	    old_hi * 100 / (old_hi + old_lo),
	    sc->sc_clkfreq / (clk_div + 1), hi, lo, hi * 100 / (hi + lo));
#endif
	return 0;
}

static pwm_tag_t
meson_pwm_get_tag(device_t dev, const void *data, size_t len)
{
	struct meson_pwm_softc * const sc = device_private(dev);
	struct meson_pwm_channel *pwmchan;
	const u_int *pwm = data;

	if (len != 16)
		return NULL;

	const u_int index = be32toh(pwm[1]);
	if (index >= MESON_PWM_NCHAN)
		return NULL;
	const u_int period = be32toh(pwm[2]);
	const u_int polarity = (pwm[3] == 0) ? PWM_ACTIVE_HIGH : PWM_ACTIVE_LOW;

	pwmchan = &sc->sc_pwmchan[index];

	/*
	 * if polarity or period in pwm-tag is different from the copy of
	 * config it holds, the content returned by pwm_get_conf() should
	 * also be according to the tag.
	 * this is because the caller may only set_conf() if necessary.
	 */
	if (pwmchan->mpc_conf.polarity != polarity) {
		pwmchan->mpc_conf.duty_cycle =
		    pwmchan->mpc_conf.period - pwmchan->mpc_conf.duty_cycle;
		pwmchan->mpc_conf.polarity = polarity;
	}
	if (pwmchan->mpc_conf.period != period) {
		if (pwmchan->mpc_conf.period == 0) {
			pwmchan->mpc_conf.duty_cycle = 0;
		} else {
			pwmchan->mpc_conf.duty_cycle =
			    (uint64_t)pwmchan->mpc_conf.duty_cycle *
			    period / pwmchan->mpc_conf.period;
		}
		pwmchan->mpc_conf.period = period;
	}

	return &pwmchan->mpc_pwm;
}

static struct fdtbus_pwm_controller_func meson_pwm_funcs = {
	.get_tag = meson_pwm_get_tag
};

static int
meson_pwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_pwm_attach(device_t parent, device_t self, void *aux)
{
	struct meson_pwm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	struct clk *clk;
	int phandle, i;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle = faa->faa_phandle;

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
	sc->sc_clkfreq = clk_get_rate(clk);

	if (clk == fdtbus_clock_byname("vid_pll"))
		sc->sc_clksource = 1;
	else if (clk == fdtbus_clock_byname("fclk_div4"))
		sc->sc_clksource = 2;
	else if (clk == fdtbus_clock_byname("fclk_div3"))
		sc->sc_clksource = 3;
	else
		sc->sc_clksource = 0;	/* default: "xtal" */

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Pulse-Width Modulation\n");

	mutex_init(&sc->sc_reglock, MUTEX_DEFAULT, IPL_NONE);
	for (i = 0; i < __arraycount(sc->sc_pwmchan); i++) {
		sc->sc_pwmchan[i].mpc_sc = sc;
		sc->sc_pwmchan[i].mpc_index = i;
		sc->sc_pwmchan[i].mpc_pwm.pwm_enable = meson_pwm_enable;
		sc->sc_pwmchan[i].mpc_pwm.pwm_get_config = meson_pwm_get_config;
		sc->sc_pwmchan[i].mpc_pwm.pwm_set_config = meson_pwm_set_config;
		sc->sc_pwmchan[i].mpc_pwm.pwm_dev = self;
		sc->sc_pwmchan[i].mpc_pwm.pwm_priv = &sc->sc_pwmchan[i];
		meson_pwm_get_current(sc, i, &sc->sc_pwmchan[i].mpc_conf);
	}

	fdtbus_register_pwm_controller(self, phandle, &meson_pwm_funcs);
}

CFATTACH_DECL_NEW(meson_pwm, sizeof(struct meson_pwm_softc),
    meson_pwm_match, meson_pwm_attach, NULL, NULL);
