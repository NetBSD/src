/* $NetBSD: rk_pwm.c,v 1.1 2019/05/01 10:41:33 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rk_pwm.c,v 1.1 2019/05/01 10:41:33 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/pwm/pwmvar.h>

#include <dev/fdt/fdtvar.h>

#define	PWM0_CNT		0x00
#define	PWM0_PERIOD_HPR		0x04
#define	PWM0_DUTY_LPR		0x08
#define	PWM0_CTRL		0x0c
#define	 CTRL_RPT		__BITS(31,24)
#define	 CTRL_SCALE		__BITS(23,16)
#define	 CTRL_PRESCALE		__BITS(14,12)
#define	 CTRL_CLK_SEL		__BIT(9)
#define	 CTRL_LP_EN		__BIT(8)
#define	 CTRL_OUTPUT_MODE	__BIT(5)
#define	 CTRL_INACTIVE_POL	__BIT(4)
#define	 CTRL_DUTY_POL		__BIT(3)
#define	 CTRL_PWM_MODE		__BITS(2,1)
#define	  CTRL_PWM_MODE_ONESHOT		0
#define	  CTRL_PWM_MODE_CONTINUOUS	1
#define	  CTRL_PWM_MODE_CAPTURE		2
#define	 CTRL_PWM_EN		__BIT(0)

enum rk_pwm_type {
	PWM_RK3288 = 1,
};

static const struct of_compat_data compat_data[] = {
	{ "rockchip,rk3288-pwm",	PWM_RK3288 },
	{ NULL }
};

struct rk_pwm_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct pwm_controller	sc_pwm;
	struct pwm_config	sc_conf;

	u_int			sc_clkfreq;
};

#define	PWM_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PWM_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static pwm_tag_t
rk_pwm_get_tag(device_t dev, const void *data, size_t len)
{
	struct rk_pwm_softc * const sc = device_private(dev);
	const u_int *pwm = data;

	if (len != 16)
		return NULL;

	const u_int index = be32toh(pwm[1]);
	if (index != 0)
		return NULL;

	const u_int period = be32toh(pwm[2]);
	const u_int polarity = be32toh(pwm[3]);

	sc->sc_conf.period = period;
	sc->sc_conf.polarity = polarity ? PWM_ACTIVE_LOW : PWM_ACTIVE_HIGH;

	return &sc->sc_pwm;
}

static struct fdtbus_pwm_controller_func rk_pwm_funcs = {
	.get_tag = rk_pwm_get_tag
};

static int
rk_pwm_enable(pwm_tag_t pwm, bool enable)
{
	struct rk_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t ctrl, octrl;

	octrl = ctrl = PWM_READ(sc, PWM0_CTRL);
	if (enable)
		ctrl |= CTRL_PWM_EN;
	else
		ctrl &= ~CTRL_PWM_EN;

	if (ctrl != octrl)
		PWM_WRITE(sc, PWM0_CTRL, ctrl);

	return 0;
}

static int
rk_pwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	struct rk_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t ctrl, period, duty;
	u_int div;

	ctrl = PWM_READ(sc, PWM0_CTRL);
	period = PWM_READ(sc, PWM0_PERIOD_HPR);
	duty = PWM_READ(sc, PWM0_DUTY_LPR);

	if (ctrl & CTRL_CLK_SEL) {
		div = __SHIFTOUT(ctrl, CTRL_SCALE) * 2;
		if (div == 0)
			div = 512;
	} else {
		div = 1;
	}
	div /= (1 << __SHIFTOUT(ctrl, CTRL_PRESCALE));

	const uint64_t rate = sc->sc_clkfreq / div;

	conf->polarity = (ctrl & CTRL_DUTY_POL) ? PWM_ACTIVE_HIGH : PWM_ACTIVE_LOW;
        conf->period = (u_int)(((uint64_t)period * 1000000000) / rate);
        conf->duty_cycle = (u_int)(((uint64_t)duty * 1000000000) / rate);

	return 0;
}

static int
rk_pwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	struct rk_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t ctrl;

	const uint64_t rate = sc->sc_clkfreq;
	const uint32_t period = (u_int)((conf->period * rate) / 1000000000);
	const uint32_t duty = (u_int)((conf->duty_cycle * rate) / 1000000000);

	/* Preserve PWM_EN flag */
	ctrl = PWM_READ(sc, PWM0_CTRL) & CTRL_PWM_EN;

	ctrl |= __SHIFTIN(CTRL_PWM_MODE_CONTINUOUS, CTRL_PWM_MODE);
	if (conf->polarity == PWM_ACTIVE_HIGH)
		ctrl |= CTRL_DUTY_POL;
	else
		ctrl |= CTRL_INACTIVE_POL;

	PWM_WRITE(sc, PWM0_CTRL, 0);
	PWM_WRITE(sc, PWM0_PERIOD_HPR, period);
	PWM_WRITE(sc, PWM0_DUTY_LPR, duty);
	PWM_WRITE(sc, PWM0_CTRL, ctrl);

	sc->sc_conf = *conf;

	return 0;
}

static int
rk_pwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
rk_pwm_attach(device_t parent, device_t self, void *aux)
{
	struct rk_pwm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_clkfreq = clk_get_rate(clk);
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIx64 ": %d",
		    (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": PWM\n");

	sc->sc_pwm.pwm_enable = rk_pwm_enable;
	sc->sc_pwm.pwm_get_config = rk_pwm_get_config;
	sc->sc_pwm.pwm_set_config = rk_pwm_set_config;
	sc->sc_pwm.pwm_dev = self;

	fdtbus_register_pwm_controller(self, phandle,
	    &rk_pwm_funcs);
}

CFATTACH_DECL_NEW(rk_pwm, sizeof(struct rk_pwm_softc),
	rk_pwm_match, rk_pwm_attach, NULL, NULL);
